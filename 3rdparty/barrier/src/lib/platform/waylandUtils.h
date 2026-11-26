// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef WAYLANDUTILS_H
#define WAYLANDUTILS_H

#include "XWindowsScreen.h"
#include "base/Log.h"
#include <dlfcn.h>  // 用于动态加载库
#include <X11/Xresource.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <type_traits>

class WaylandUtils
{
public:
    // 定义函数指针类型
    typedef int (*InitKvmFunc)();
    typedef void (*KvmRegisterPointerMotionFunc)(void*, void (*)(void*, unsigned int, double, double));
    typedef void (*KvmPointerSetPosFunc)(double, double);

    // 静态成员变量，用于存储函数指针
    static KvmPointerSetPosFunc s_kvmPointerSetPos;

    // Wayland 相关的共享状态（屏幕尺寸、缩放、回显标记）会被 X 事件线程和
    // libdisplayjack 的回调线程同时访问，统一用这把递归锁保护。
    static std::recursive_timed_mutex s_callbackMutex;

    // displayjack 的 kvm 事件使用 kwin 的逻辑坐标(已按缩放换算)，
    // 而 barrier 的屏幕尺寸取自 XWayland 的物理坐标，两者相差一个缩放倍数，
    // 缩放不为 1 时指针永远到不了屏幕边缘，需要在这里做换算。
    // 非整数缩放时逻辑尺寸是物理尺寸除以缩放后取整(例如 2160/1.75 = 1234)，
    // 直接乘缩放倍数会少 1 个像素(1233*1.75 = 2157.75)导致到不了边缘，
    // 因此按 [0, 逻辑尺寸-1] 到 [0, 物理尺寸-1] 做线性映射。
    static double s_scaleFactor;
    static int s_physicalWidth;
    static int s_physicalHeight;
    static int s_logicalWidth;
    static int s_logicalHeight;

    // kvm 回调运行在 libdisplayjack 自己的 dispatch 线程上。屏幕对象析构、
    // 事件队列销毁期间必须拦住在途/后续回调，否则会访问已释放的 EventQueue
    // 导致 SIGSEGV。
    static std::atomic<bool> s_active;
    // 当前有效的屏幕对象。回调里先比对，避免旧 dispatch 线程
    // 在屏障重建后带着已析构的 user_data 继续进来。
    static std::atomic<void *> s_screen;

    // 注销回调靠库自己的 destory_kvm()（会停掉它的 dispatch 线程），
    // 这里再用引用计数 + 条件变量等在途回调归零，避免"超时后继续析构"。
    typedef void (*DestroyKvmFunc)();
    static DestroyKvmFunc s_destroyKvm;
    static std::atomic<int> s_inFlight;
    static std::mutex s_drainMutex;
    static std::condition_variable s_drainCond;

    // 初始化动态库和函数指针，并处理回调
    static bool initWaylandKvm(void *user_data, Display *display = nullptr)
    {
        // 先取 X 物理尺寸，再据此确定缩放，最后算出逻辑尺寸；
        // 全部用局部值算完后一次性发布，避免回调读到"半新半旧"的状态。
        int physicalWidth = 0;
        int physicalHeight = 0;
        getPhysicalSize(user_data, physicalWidth, physicalHeight);

        const double scale = detectScaleFactor(display, physicalWidth, physicalHeight);
        int logicalWidth = 0;
        int logicalHeight = 0;
        logicalSizeFor(physicalWidth, physicalHeight, scale, logicalWidth, logicalHeight);

        {
            std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
            s_physicalWidth = physicalWidth;
            s_physicalHeight = physicalHeight;
            s_scaleFactor = scale;
            s_logicalWidth = logicalWidth;
            s_logicalHeight = logicalHeight;
            s_relAccumX = 0.0;
            s_relAccumY = 0.0;
        }
        LOG((CLOG_NOTE "kvm screen: physical %dx%d, logical %dx%d, scale %.2f",
             physicalWidth, physicalHeight, logicalWidth, logicalHeight, scale));

        void* kvmLibHandle = dlopen("libdisplayjack_kvm.so", RTLD_LAZY);
        if (!kvmLibHandle) {
            LOG((CLOG_NOTE "Failed to load libdisplayjack_kvm.so: %s", dlerror()));
            return false;
        }

        InitKvmFunc initKvm = (InitKvmFunc)dlsym(kvmLibHandle, "init_kvm");
        KvmRegisterPointerMotionFunc kvmRegisterPointerMotion = (KvmRegisterPointerMotionFunc)dlsym(kvmLibHandle, "kvm_register_pointer_motion");
        s_kvmPointerSetPos = (KvmPointerSetPosFunc)dlsym(kvmLibHandle, "kvm_pointer_set_pos");
        s_destroyKvm = (DestroyKvmFunc)dlsym(kvmLibHandle, "destory_kvm");

        if (!initKvm || !kvmRegisterPointerMotion || !s_kvmPointerSetPos) {
            LOG((CLOG_NOTE "Failed to resolve symbols: %s", dlerror()));
            // 库即将被卸载，悬空的函数指针必须清掉
            s_kvmPointerSetPos = nullptr;
            s_destroyKvm = nullptr;
            dlclose(kvmLibHandle);
            return false;
        }

        int initSucceed = initKvm();
        LOG((CLOG_NOTE " init kvm status: %d", initSucceed));

        if (initSucceed == 0) {
            // 先注册再置 active：注册前的回调会因 !active 直接返回
            kvmRegisterPointerMotion(user_data, WaylandUtils::kvmPointerMotionCallback);
            {
                std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
                s_screen.store(user_data, std::memory_order_release);
                s_active.store(true, std::memory_order_release);
            }
        }

        return true;
    }

    // 屏幕对象析构前调用，必须保证返回后不再有回调访问屏幕对象：
    // 1) 先把 active 置 false，让后续回调进到入口就退出；
    // 2) 调库的 destory_kvm() 停掉它的 dispatch 线程，保证不会再产生新回调；
    // 3) 取一次 s_callbackMutex，证明没有回调卡在"已通过 active 检查"的区间；
    // 4) 等在途回调把引用计数减到 0。
    // 回调里唯一的阻塞点是 X 往返(XSync)，X server 不响应时进程本身也已不可用；
    // kvm_pointer_set_pos 是异步请求(回调线程自己也在调它)，不会等 dispatch 线程。
    static void shutdownKvm()
    {
        s_active.store(false, std::memory_order_release);
        s_screen.store(nullptr, std::memory_order_release);

        if (s_destroyKvm) {
            DestroyKvmFunc destroyKvm = s_destroyKvm;
            s_destroyKvm = nullptr;
            destroyKvm();   // 会 wl_display_disconnect，之后 set_pos 指针不可再使用
        } else {
            LOG((CLOG_NOTE "kvm: destory_kvm is not available, drain callbacks only"));
        }

        {
            std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
            // 库已断开，清掉注入入口避免后续 warp/fakeMouseMove 调用已释放的 display
            s_kvmPointerSetPos = nullptr;
        }

        // 回调从"读 s_active"到"登记 inFlight"再到调用 handleMouseMove 全程持有
        // s_callbackMutex。这里拿一次锁，即证明不存在"已读到 active=true 但还没
        // 登记 inFlight"的回调（它们要么已跑完并递减，要么还没开始读）。
        {
            std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        }

        std::unique_lock<std::mutex> lock(s_drainMutex);
        s_drainCond.wait(lock, [] {
            return s_inFlight.load(std::memory_order_acquire) == 0;
        });
    }

    static void kvmPointerMotionCallback(void *user_data, unsigned int time, double x, double y)
    {
        XWindowsScreen *xsreen = static_cast<XWindowsScreen *>(user_data);
        if (!xsreen)
            return;

        std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        if (!s_active.load(std::memory_order_acquire))
            return;
        // 屏幕对象已析构/已换新时，旧线程的回调直接丢弃
        if (s_screen.load(std::memory_order_acquire) != user_data)
            return;

        // 登记"在途"，析构时等到它归零
        s_inFlight.fetch_add(1, std::memory_order_acq_rel);

        struct InFlightGuard {
            ~InFlightGuard()
            {
                if (s_inFlight.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    std::lock_guard<std::mutex> lock(s_drainMutex);
                    s_drainCond.notify_all();
                }
            }
        } inFlightGuard;

        // kvm 上报逻辑坐标，换算成 barrier 使用的 X 物理坐标
        const int xRoot = logicalToPhysical(x, s_logicalWidth, s_physicalWidth);
        const int yRoot = logicalToPhysical(y, s_logicalHeight, s_physicalHeight);

        XMotionEvent xmotion;
        xmotion.type = MotionNotify;
        xmotion.send_event = False;   // Raw motion
        xmotion.x_root = xRoot;
        xmotion.y_root = yRoot;
        xmotion.x = xRoot;
        xmotion.y = yRoot;
        xsreen->handleMouseMove(xmotion);
    }

    static bool setPointerPos(double x, double y)
    {
        // 锁内只取快照和换算坐标，尽量缩短持锁区间。
        // kvm_pointer_set_pos 是异步请求（不等待应答），不会依赖 dispatch 线程。
        KvmPointerSetPosFunc setPos = nullptr;
        double logicalX = 0.0;
        double logicalY = 0.0;
        {
            std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
            // 停机后 kvm 库已断开，不能再调
            if (!s_active.load(std::memory_order_acquire))
                return false;
            setPos = s_kvmPointerSetPos;
            // barrier 传入 X 物理坐标，kvm 接口需要逻辑坐标
            logicalX = physicalToLogical(x, s_physicalWidth, s_logicalWidth);
            logicalY = physicalToLogical(y, s_physicalHeight, s_logicalHeight);
        }

        if (!setPos)
            return false;

        setPos(logicalX, logicalY);
        return true;
    }

    static bool isWayland()
    {
        // 会话类型在进程生命周期内不变，缓存一次即可
        static const bool cached = []() {
            const char *type = getenv("XDG_SESSION_TYPE");
            if (type && strcmp(type, "wayland") == 0)
                return true;
            // WAYLAND_DISPLAY 的实际值是 wayland-0 或 socket 绝对路径，
            // 不能用 "wayland" 全等比较，按包含关系判断
            const char *display = getenv("WAYLAND_DISPLAY");
            return display && strstr(display, "wayland") != nullptr;
        }();
        return cached;
    }

    // 客户端注入：kwin 把 XTest 坐标当作合成器逻辑坐标处理，
    // barrier 传入的是 X 物理坐标，注入前需要做同样的换算，
    // 否则非 100% 缩放时光标会被放大导致漂移和来回切屏。
    // X11 会话下缩放为 1、逻辑尺寸等于物理尺寸，这组换算是恒等的，
    // 行为与改动前一致（是否走这些分支由调用方的 isWayland() 决定）。
    static void physicalToDevice(SInt32 &x, SInt32 &y)
    {
        std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        x = static_cast<SInt32>(std::lround(physicalToLogical(x, s_physicalWidth, s_logicalWidth)));
        y = static_cast<SInt32>(std::lround(physicalToLogical(y, s_physicalHeight, s_logicalHeight)));
    }

    static void physicalDeltaToDevice(SInt32 &dx, SInt32 &dy)
    {
        std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        const double scale = (s_scaleFactor > 0.0) ? s_scaleFactor : 1.0;

        // 逐事件 lround(dx/scale) 在 175%/200% 下会系统性少走，
        // 这里把余数累加到下一次，保证总位移守恒
        s_relAccumX += dx;
        s_relAccumY += dy;
        const SInt32 outX = static_cast<SInt32>(std::lround(s_relAccumX / scale));
        const SInt32 outY = static_cast<SInt32>(std::lround(s_relAccumY / scale));
        s_relAccumX -= outX * scale;
        s_relAccumY -= outY * scale;
        dx = outX;
        dy = outY;
    }

    // 我们自己 warp/注入光标后，合成器会把新位置当作一次指针移动上报回来。
    // X11 下 barrier 依靠 XSendEvent 标记来丢弃这类回显，Wayland 下没有这个标记，
    // 不做处理会算出虚假位移，导致非整数缩放下反复切屏。
    // 调用方都在 Wayland 分支里，这里不再判断会话类型（便于单元测试）
    static void markSyntheticPosition(SInt32 x, SInt32 y)
    {
        std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        SyntheticMark &mark = s_synthMarks[s_synthMarkNext];
        s_synthMarkNext = (s_synthMarkNext + 1) % kSyntheticMarkCount;
        mark.x = x;
        mark.y = y;
        mark.time = std::chrono::steady_clock::now();
        mark.pending = true;
    }

    // 命中一次即解除该标记：只吞掉我们自己那次 warp 的回显，
    // 同一位置上后续的真实位移不会被重复丢弃。
    static bool isSyntheticEcho(SInt32 x, SInt32 y)
    {
        // 调用方可能在 kvm 回调线程上，锁是递归的
        std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
        const auto now = std::chrono::steady_clock::now();
        for (SyntheticMark &mark : s_synthMarks) {
            if (!mark.pending)
                continue;
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                     now - mark.time).count();
            if (elapsed > kEchoWindowMs) {
                mark.pending = false;
                continue;
            }
            if (std::abs(x - mark.x) <= kEchoTolerancePx &&
                std::abs(y - mark.y) <= kEchoTolerancePx) {
                mark.pending = false;
                return true;
            }
        }
        return false;
    }

    // 屏幕尺寸/缩放变化后刷新缓存（热插拔、热缩放）
    static void refreshScreen(Display *display, SInt32 width, SInt32 height)
    {
        if (!isWayland() || width <= 0 || height <= 0)
            return;

        // 探测要连合成器，放锁外做：全程用局部值算完，最后在一次锁内
        // 同时发布物理尺寸、缩放与逻辑尺寸。否则"新物理尺寸 + 旧缩放"
        // 的瞬态会被 kvm 回调读到，算出错误比例。
        const double scale = detectScaleFactor(display, width, height);
        int logicalWidth = 0;
        int logicalHeight = 0;
        logicalSizeFor(width, height, scale, logicalWidth, logicalHeight);

        {
            std::lock_guard<std::recursive_timed_mutex> lock(s_callbackMutex);
            s_physicalWidth = width;
            s_physicalHeight = height;
            s_scaleFactor = scale;
            s_logicalWidth = logicalWidth;
            s_logicalHeight = logicalHeight;
            s_relAccumX = 0.0;
            s_relAccumY = 0.0;
        }

        // 日志放在锁外，避免持锁做格式化与输出
        LOG((CLOG_NOTE "kvm screen changed: physical %dx%d, logical %dx%d, scale %.2f",
             width, height, logicalWidth, logicalHeight, scale));
    }

private:
    // 单元测试通过这个 peer 访问内部纯函数，避免用 #define private public
    // 这种会改变访问控制、影响标准库头解析的做法。
    friend struct WaylandUtilsTestPeer;

    // 自己 warp/注入的位置记录，用来识别合成器报回来的回显
    struct SyntheticMark {
        SInt32 x;
        SInt32 y;
        std::chrono::steady_clock::time_point time;
        bool pending;
    };
    enum SyntheticMarkCount { kSyntheticMarkCount = 4 };
    static SyntheticMark s_synthMarks[kSyntheticMarkCount];
    static int s_synthMarkNext;

    // 相对位移换算的余数，避免逐事件取整造成系统性偏差
    static double s_relAccumX;
    static double s_relAccumY;

    static void getPhysicalSize(void *user_data, int &width, int &height)
    {
        width = 0;
        height = 0;

        XWindowsScreen *screen = static_cast<XWindowsScreen *>(user_data);
        if (!screen)
            return;

        SInt32 x = 0;
        SInt32 y = 0;
        SInt32 w = 0;
        SInt32 h = 0;
        screen->getShape(x, y, w, h);
        if (w <= 0 || h <= 0)
            return;

        width = w;
        height = h;
    }

    static void logicalSizeFor(int physicalWidth, int physicalHeight, double scale,
                               int &logicalWidth, int &logicalHeight)
    {
        logicalWidth = 0;
        logicalHeight = 0;
        if (physicalWidth <= 0 || physicalHeight <= 0 || scale <= 0.0)
            return;

        logicalWidth = static_cast<int>(std::lround(physicalWidth / scale));
        logicalHeight = static_cast<int>(std::lround(physicalHeight / scale));
    }

    // 把坐标从 [0, fromSize-1] 线性映射到 [0, toSize-1]。
    // 逻辑/物理两个方向的换算都走这里，参数统一为 (值, 源尺寸, 目标尺寸)。
    static double mapCoordinate(double value, int fromSize, int toSize)
    {
        if (fromSize > 1 && toSize > 1)
            return value * (toSize - 1) / (fromSize - 1);
        return value;
    }

    // 逻辑坐标 -> X 物理坐标（kvm 上报 -> barrier）
    static int logicalToPhysical(double value, int logicalSize, int physicalSize)
    {
        if (logicalSize > 1 && physicalSize > 1) {
            // 非整数缩放下逻辑尺寸是取整后的值，逻辑边缘要精确落到物理边缘。
            // 靠近边缘的几个像素直接吸附到 physicalSize-1，吸收取整误差。
            if (s_scaleFactor > 1.0 && value >= logicalSize - kEdgeSnapLogicalPx)
                return physicalSize - 1;
            return static_cast<int>(std::lround(mapCoordinate(value, logicalSize, physicalSize)));
        }
        return static_cast<int>(std::lround(value * s_scaleFactor));
    }

    // X 物理坐标 -> 逻辑坐标（barrier 注入 -> kvm）
    static double physicalToLogical(double value, int physicalSize, int logicalSize)
    {
        if (physicalSize > 1 && logicalSize > 1)
            return mapCoordinate(value, physicalSize, logicalSize);
        return value / s_scaleFactor;
    }

    static double sanitizeScale(double scale)
    {
        if (!std::isfinite(scale) || scale < 1.0 || scale > 5.0)
            return 1.0;
        return scale;
    }

    // ---------- 向合成器查询每个输出的逻辑/物理尺寸 ----------
    // xdg_output 给逻辑矩形，wl_output.mode 给物理尺寸；两者一致时缩放即物理/逻辑。
    // 这是"实际生效"的缩放，不依赖 DDE 配置文件（配置可能滞后于 kvm 实际状态）。
    struct WlMessageDef {
        const char *name;
        const char *signature;
        const void *types;
    };

    struct WlInterfaceDef {
        const char *name;
        int version;
        int methodCount;
        const WlMessageDef *methods;
        int eventCount;
        const WlMessageDef *events;
    };

    // 这两个结构是 libwayland 的 wl_message / wl_interface 的手写副本。
    // 布局一旦漂移，运行时就是内存错乱，所以把字段偏移固定在这里。
    static_assert(sizeof(WlMessageDef) == 3 * sizeof(void *),
                  "wl_message layout changed: name/signature/types must stay pointer sized");
    static_assert(offsetof(WlInterfaceDef, version) == sizeof(const char *),
                  "wl_interface layout changed: version must follow name");
    static_assert(offsetof(WlInterfaceDef, methodCount) == sizeof(const char *) + sizeof(int),
                  "wl_interface layout changed: methodCount must follow version");
    static_assert(offsetof(WlInterfaceDef, methods) == sizeof(const char *) + 2 * sizeof(int),
                  "wl_interface layout changed: methods must follow methodCount");
    static_assert(
        offsetof(WlInterfaceDef, eventCount) ==
            sizeof(const char *) + 2 * sizeof(int) + sizeof(const void *),
        "wl_interface layout changed: eventCount must follow methods");
    static_assert(
        offsetof(WlInterfaceDef, events) ==
            (offsetof(WlInterfaceDef, eventCount) + sizeof(int) + alignof(void *) - 1) /
                alignof(void *) * alignof(void *),
        "wl_interface layout changed: events must follow eventCount without extra padding");
    static_assert(std::is_standard_layout<WlMessageDef>::value &&
                      std::is_standard_layout<WlInterfaceDef>::value,
                  "libwayland struct copies must stay standard layout");

    struct WlRegistryListenerDef {
        void (*global)(void *, void *, uint32_t, const char *, uint32_t);
        void (*globalRemove)(void *, void *, uint32_t);
    };

    struct WlXdgOutputListenerDef {
        void (*logicalPosition)(void *, void *, int32_t, int32_t);
        void (*logicalSize)(void *, void *, int32_t, int32_t);
        void (*done)(void *, void *);
        void (*name)(void *, void *, const char *);
        void (*description)(void *, void *, const char *);
    };

    struct WlOutputListenerDef {
        void (*geometry)(void *, void *, int32_t, int32_t, int32_t, int32_t,
                         int32_t, const char *, const char *, int32_t);
        void (*mode)(void *, void *, uint32_t, int32_t, int32_t, int32_t);
        void (*done)(void *, void *);
        void (*scale)(void *, void *, int32_t);
        void (*name)(void *, void *, const char *);
        void (*description)(void *, void *, const char *);
    };

    enum { kMaxCompositorOutputs = 8 };
    enum { kWlOutputModeCurrent = 0x1 };
    enum { kEchoWindowMs = 150 };
    enum { kEchoTolerancePx = 1 };
    // 逻辑边缘吸附窗口：非整数缩放下逻辑尺寸取整会少 1~2 px，
    // 靠近边缘的这几点直接吸附到物理边缘，保证能切屏。
    enum { kEdgeSnapLogicalPx = 2 };

    struct OutputBox {
        int32_t x;
        int32_t y;
        int32_t w;
        int32_t h;
        int32_t physW;
        int32_t physH;
        bool hasPosition;
        bool hasSize;
        bool hasMode;
    };

    // libwayland-client 只加载一次并常驻进程：查询只在初始化和 RANDR 变化时发生，
    // dlopen/dlclose 实测约 60us，但没必要反复做。wl_display 不缓存——长连接
    // 需要持续的 dispatch 才能保持有效状态，每次查询重新连接更安全。
    struct WaylandClientApi {
        void *(*displayConnect)(const char *);
        void (*displayDisconnect)(void *);
        int (*displayRoundtrip)(void *);
        void *(*marshal)(void *, uint32_t, const void *, uint32_t, ...);
        int (*proxyAddListener)(void *, const void *, void *);
        const void *outputInterface;
        const void *registryInterface;
    };

    static const WaylandClientApi *waylandClientApi()
    {
        static const WaylandClientApi api = []() {
            WaylandClientApi a{};
            void *lib = dlopen("libwayland-client.so.0", RTLD_LAZY);
            if (!lib) {
                LOG((CLOG_WARN "kvm: cannot load libwayland-client.so.0: %s", dlerror()));
                return a;
            }
            a.displayConnect =
                reinterpret_cast<void *(*)(const char *)>(dlsym(lib, "wl_display_connect"));
            a.displayDisconnect =
                reinterpret_cast<void (*)(void *)>(dlsym(lib, "wl_display_disconnect"));
            a.displayRoundtrip = reinterpret_cast<int (*)(void *)>(dlsym(lib, "wl_display_roundtrip"));
            a.marshal = reinterpret_cast<void *(*)(void *, uint32_t, const void *, uint32_t, ...)>(
                dlsym(lib, "wl_proxy_marshal_constructor_versioned"));
            a.proxyAddListener =
                reinterpret_cast<int (*)(void *, const void *, void *)>(dlsym(lib, "wl_proxy_add_listener"));
            a.outputInterface = dlsym(lib, "wl_output_interface");
            a.registryInterface = dlsym(lib, "wl_registry_interface");

            if (!a.displayConnect || !a.displayDisconnect || !a.displayRoundtrip || !a.marshal ||
                !a.proxyAddListener || !a.outputInterface || !a.registryInterface) {
                LOG((CLOG_WARN "kvm: libwayland-client is missing required symbols"));
                // 不 dlclose：句柄常驻，避免已取到的符号指针悬空
                return WaylandClientApi{};
            }
            return a;
        }();
        return &api;
    }

    struct CompositorQuery {
        const WaylandClientApi *api;
        void *manager;
        uint32_t managerVersion;
        void *outputs[kMaxCompositorOutputs];
        OutputBox boxes[kMaxCompositorOutputs];
        int outputCount;
    };

    static const WlInterfaceDef *xdgOutputInterfaceDef()
    {
        static const WlMessageDef requests[] = {
            { "destroy", "", nullptr },
        };
        // name/description 是 zxdg-output 协议里 since=2 的事件，签名里的 "2"
        // 是 wayland-scanner 对 since>1 消息加的参数版本前缀（可对照 libwayland
        // 的 wl_output：done="2"、scale="2i"、name="4s"），不是笔误。
        static const WlMessageDef events[] = {
            { "logical_position", "ii", nullptr },
            { "logical_size", "ii", nullptr },
            { "done", "", nullptr },
            { "name", "2s", nullptr },
            { "description", "2s", nullptr },
        };
        static const WlInterfaceDef def = { "zxdg_output_v1", 3, 1, requests, 5, events };
        return &def;
    }

    static const WlInterfaceDef *xdgOutputManagerInterfaceDef(const void *outputInterface)
    {
        static WlMessageDef requests[2];
        static const void *getTypes[2];
        static const WlInterfaceDef def = { "zxdg_output_manager_v1", 3, 2, requests, 0, nullptr };

        // 接口表里嵌了运行时取到的 wl_output 接口指针，只能初始化一次；
        // call_once 保证并发查询时不会一边读一边改写。
        static std::once_flag once;
        std::call_once(once, [outputInterface]() {
            getTypes[0] = xdgOutputInterfaceDef();
            getTypes[1] = outputInterface;
            requests[0] = { "destroy", "", nullptr };
            requests[1] = { "get_xdg_output", "no", getTypes };
        });
        return &def;
    }

    // wl_registry.bind / wl_display.get_registry 在头文件里是 inline 的，
    // 库里没有导出，这里用 marshal 直接构造
    static void *bindGlobal(CompositorQuery &query, void *registry, uint32_t name,
                            const void *interface, const char *interfaceName, uint32_t version)
    {
        return query.api->marshal(registry, 0 /* WL_REGISTRY_BIND */, interface, version,
                                  name, interfaceName, version, nullptr);
    }

    enum { kWlOutputEventCount = 6 };
    enum { kWlOutputEventCountRequired = 4 };   // geometry/mode/done/scale
    enum { kXdgOutputEventCount = 5 };
    enum { kRegistryEventCount = 2 };

    // libwayland 按事件序号索引监听槽，槽位数量/顺序错了会调错函数或越界。
    static_assert(sizeof(WlOutputListenerDef) == kWlOutputEventCount * sizeof(void (*)(void)),
                  "wl_output has 6 events, the listener must provide one slot per event");
    static_assert(sizeof(WlXdgOutputListenerDef) == kXdgOutputEventCount * sizeof(void (*)(void)),
                  "zxdg_output_v1 has 5 events, the listener must provide one slot per event");
    static_assert(sizeof(WlRegistryListenerDef) == kRegistryEventCount * sizeof(void (*)(void)),
                  "wl_registry has 2 events, the listener must provide one slot per event");

    // 我们依赖的 wl_output 事件表（与 libwayland 导出的接口逐条比对用）。
    // 签名按 wayland-scanner 的形式带版本前缀，实测 libwayland 1.21(x86)/1.26(arm64)
    // 均为 geometry="iiiiissi" mode="uiii" done="2" scale="2i" name="4s" description="4s"。
    static const WlInterfaceDef *wlOutputInterfaceDef()
    {
        static const WlMessageDef events[] = {
            { "geometry", "iiiiissi", nullptr },
            { "mode", "uiii", nullptr },
            { "done", "2", nullptr },
            { "scale", "2i", nullptr },
            { "name", "4s", nullptr },
            { "description", "4s", nullptr },
        };
        static const WlInterfaceDef def = { "wl_output", 4, 0, nullptr, kWlOutputEventCount, events };
        return &def;
    }

    // 监听槽位与库里真实接口的事件表必须一致：数量不能少于所需事件，
    // 名称/签名必须逐条相同。libwayland 升级或协议漂移时在这里拦下，
    // 退回到 Xft.dpi/1.0，而不是让错的槽位在运行时被调用。
    static bool interfaceMatches(const WlInterfaceDef *expected, const void *realInterface)
    {
        if (!expected || !realInterface)
            return false;
        const WlInterfaceDef *real = static_cast<const WlInterfaceDef *>(realInterface);
        if (real->eventCount < kWlOutputEventCountRequired ||
            real->eventCount > expected->eventCount)
            return false;
        for (int i = 0; i < real->eventCount; ++i) {
            const WlMessageDef &want = expected->events[i];
            const WlMessageDef &got = real->events[i];
            if (!want.name || !want.signature || !got.name || !got.signature)
                return false;
            if (std::strcmp(want.name, got.name) != 0 ||
                std::strcmp(want.signature, got.signature) != 0)
                return false;
        }
        return true;
    }

    // libwayland 不做校验，监听槽为 NULL 时直接 wl_abort（第 5 轮 C01 即此）。
    // 绑定前显式确认每个槽位都已填充，避免以后改结构时漏掉一个。
    static bool listenersComplete()
    {
        const WlRegistryListenerDef &registry = registryListenerDef();
        const WlOutputListenerDef &output = outputListenerDef();
        const WlXdgOutputListenerDef &xdg = xdgOutputListenerDef();
        return registry.global && registry.globalRemove && output.geometry && output.mode &&
               output.done && output.scale && output.name && output.description &&
               xdg.logicalPosition && xdg.logicalSize && xdg.done && xdg.name && xdg.description;
    }

    // libwayland 派发事件时会直接调用监听槽，NULL 会 wl_abort；
    // 这里给每个事件都提供签名匹配的空实现（只用 mode）。
    static void outputGeometryCb(void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t,
                                 const char *, const char *, int32_t) {}
    static void outputScaleCb(void *, void *, int32_t) {}
    static void outputNameCb(void *, void *, const char *) {}
    static void outputDescriptionCb(void *, void *, const char *) {}

    static const WlOutputListenerDef &outputListenerDef()
    {
        static const WlOutputListenerDef listener = {
            &WaylandUtils::outputGeometryCb,
            &WaylandUtils::outputModeCb,
            &WaylandUtils::outputDoneCb,
            &WaylandUtils::outputScaleCb,
            &WaylandUtils::outputNameCb,
            &WaylandUtils::outputDescriptionCb,
        };
        return listener;
    }

    static void outputModeCb(void *data, void *, uint32_t flags, int32_t width, int32_t height,
                             int32_t)
    {
        // 只取当前模式(WL_OUTPUT_MODE_CURRENT)
        if ((flags & kWlOutputModeCurrent) == 0)
            return;
        OutputBox *box = static_cast<OutputBox *>(data);
        box->physW = width;
        box->physH = height;
        box->hasMode = true;
    }

    static void outputDoneCb(void *, void *) {}

    static void registryGlobalCb(void *data, void *registry, uint32_t name,
                                 const char *interface, uint32_t version)
    {
        CompositorQuery *query = static_cast<CompositorQuery *>(data);
        if (!interface)
            return;

        if (std::strcmp(interface, "zxdg_output_manager_v1") == 0 && !query->manager) {
            query->managerVersion = std::min<uint32_t>(version, 3);
            query->manager = bindGlobal(*query, registry, name,
                                        xdgOutputManagerInterfaceDef(query->api->outputInterface),
                                        "zxdg_output_manager_v1", query->managerVersion);
        } else if (std::strcmp(interface, "wl_output") == 0) {
            if (query->outputCount >= kMaxCompositorOutputs) {
                LOG((CLOG_WARN "kvm: too many outputs, ignore extra one"));
                return;
            }
            const uint32_t outputVersion = std::min<uint32_t>(version, 4);
            void *output = bindGlobal(*query, registry, name, query->api->outputInterface,
                                      "wl_output", outputVersion);
            if (!output)
                return;

            OutputBox &box = query->boxes[query->outputCount];
            box = OutputBox{};
            // wl_output 的 mode 事件给出物理尺寸，用来和逻辑尺寸算每屏缩放
            query->api->proxyAddListener(output, &outputListenerDef(), &box);
            query->outputs[query->outputCount++] = output;
        }
    }

    static void registryGlobalRemoveCb(void *, void *, uint32_t) {}

    static void xdgLogicalPositionCb(void *data, void *, int32_t x, int32_t y)
    {
        OutputBox *box = static_cast<OutputBox *>(data);
        box->x = x;
        box->y = y;
        box->hasPosition = true;
    }

    static void xdgLogicalSizeCb(void *data, void *, int32_t w, int32_t h)
    {
        OutputBox *box = static_cast<OutputBox *>(data);
        box->w = w;
        box->h = h;
        box->hasSize = true;
    }

    static void xdgDoneCb(void *, void *) {}
    static void xdgNameCb(void *, void *, const char *) {}
    static void xdgDescriptionCb(void *, void *, const char *) {}

    static const WlRegistryListenerDef &registryListenerDef()
    {
        static const WlRegistryListenerDef listener = {
            &WaylandUtils::registryGlobalCb, &WaylandUtils::registryGlobalRemoveCb
        };
        return listener;
    }

    static const WlXdgOutputListenerDef &xdgOutputListenerDef()
    {
        static const WlXdgOutputListenerDef listener = {
            &WaylandUtils::xdgLogicalPositionCb, &WaylandUtils::xdgLogicalSizeCb,
            &WaylandUtils::xdgDoneCb, &WaylandUtils::xdgNameCb, &WaylandUtils::xdgDescriptionCb
        };
        return listener;
    }

    // 每屏缩放 = 物理尺寸/逻辑尺寸。90°/270° 旋转时 kwin 报的 mode 是未旋转的
    // 物理尺寸、逻辑矩形是旋转后的，宽高需要对调：把正常配对与旋转配对各算一组，
    // 取差异更小的一组，避免旋转屏被误判成混合缩放。
    static double outputScaleFor(const OutputBox &box)
    {
        if (!box.hasMode || box.physW <= 0 || box.physH <= 0 || box.w <= 0 || box.h <= 0)
            return 0.0;

        const double sx = static_cast<double>(box.physW) / box.w;
        const double sy = static_cast<double>(box.physH) / box.h;
        const double rx = static_cast<double>(box.physW) / box.h;
        const double ry = static_cast<double>(box.physH) / box.w;
        const double normalDiff = std::fabs(sx - sy) / std::max(sx, sy);
        const double rotatedDiff = std::fabs(rx - ry) / std::max(rx, ry);
        return rotatedDiff < normalDiff ? (rx + ry) / 2.0 : (sx + sy) / 2.0;
    }

    // 汇总各屏逻辑矩形：并集即整体逻辑尺寸，同时判断各屏缩放是否一致。
    // 返回 >0 统一缩放；-1 各屏缩放不一致（不可换算）；0 没有可用输出。
    static double aggregateOutputs(const OutputBox *boxes, int count,
                                   int &logicalWidth, int &logicalHeight)
    {
        int minX = INT_MAX;
        int minY = INT_MAX;
        int maxX = INT_MIN;
        int maxY = INT_MIN;
        int valid = 0;
        bool mixed = false;
        double firstScale = 0.0;

        for (int i = 0; i < count; ++i) {
            const OutputBox &box = boxes[i];
            if (!box.hasSize || box.w <= 0 || box.h <= 0)
                continue;
            const int bx = box.hasPosition ? box.x : 0;
            const int by = box.hasPosition ? box.y : 0;
            minX = std::min(minX, bx);
            minY = std::min(minY, by);
            maxX = std::max(maxX, bx + box.w);
            maxY = std::max(maxY, by + box.h);
            ++valid;

            const double outputScale = outputScaleFor(box);
            if (outputScale <= 0.0)
                continue;
            if (firstScale <= 0.0)
                firstScale = outputScale;
            else if (std::fabs(outputScale - firstScale) > 0.01)
                mixed = true;
        }

        if (valid == 0)
            return 0.0;
        logicalWidth = maxX - minX;
        logicalHeight = maxY - minY;
        if (logicalWidth <= 0 || logicalHeight <= 0)
            return 0.0;
        return mixed ? -1.0 : 1.0;
    }

    // 返回：>0 统一缩放；-1 各屏缩放不一致（不可换算）；0 查询失败
    static double compositorScale(int &logicalWidth, int &logicalHeight)
    {
        const WaylandClientApi *api = waylandClientApi();
        if (!api->displayConnect)
            return 0.0;
        if (!listenersComplete()) {
            LOG((CLOG_WARN "kvm: incomplete Wayland listener, skip compositor query"));
            return 0.0;
        }
        if (!interfaceMatches(wlOutputInterfaceDef(), api->outputInterface)) {
            LOG((CLOG_WARN "kvm: wl_output interface differs from the expected protocol, skip"));
            return 0.0;
        }

        CompositorQuery query{};
        query.api = api;

        void *display = api->displayConnect(nullptr);
        if (!display) {
            LOG((CLOG_WARN "kvm: cannot connect to the Wayland compositor"));
            return 0.0;
        }

        // wl_display.get_registry -> wl_registry
        void *registry = api->marshal(display, 1 /* WL_DISPLAY_GET_REGISTRY */,
                                      api->registryInterface, 1, nullptr);

        double result = 0.0;
        if (registry) {
            api->proxyAddListener(registry, &registryListenerDef(), &query);
            api->displayRoundtrip(display);   // 取 global 列表
            api->displayRoundtrip(display);   // 取 wl_output 的 mode
        }

        if (query.manager && query.outputCount > 0) {
            for (int i = 0; i < query.outputCount; ++i) {
                void *xdgOutput = api->marshal(query.manager, 1 /* get_xdg_output */,
                                               xdgOutputInterfaceDef(), query.managerVersion,
                                               nullptr, query.outputs[i]);
                if (xdgOutput)
                    api->proxyAddListener(xdgOutput, &xdgOutputListenerDef(), &query.boxes[i]);
            }

            api->displayRoundtrip(display);   // 取 logical_position/logical_size

            result = aggregateOutputs(query.boxes, query.outputCount, logicalWidth, logicalHeight);

            // 成功路径把实际数值落日志：线上才能判断"缩放算得对不对"
            LOG((CLOG_NOTE "kvm compositor: manager v%u, outputs=%d, logical %dx%d",
                 query.managerVersion, query.outputCount, logicalWidth, logicalHeight));
            for (int i = 0; i < query.outputCount; ++i) {
                const OutputBox &box = query.boxes[i];
                LOG((CLOG_NOTE "kvm output[%d]: mode %dx%d, logical %dx%d at %d,%d", i,
                     box.physW, box.physH, box.w, box.h, box.x, box.y));
            }
        } else {
            LOG((CLOG_WARN "kvm: no zxdg_output_manager_v1/wl_output, outputs=%d",
                 query.outputCount));
        }

        api->displayDisconnect(display);
        return result;
    }

    // 合成器逻辑尺寸与 X 物理尺寸的比例即缩放倍数
    static double scaleFromCompositor(int physicalWidth, int physicalHeight)
    {
        int logicalWidth = 0;
        int logicalHeight = 0;
        const double status = compositorScale(logicalWidth, logicalHeight);
        if (status <= 0.0)
            return status;   // 0 查询失败，-1 各屏缩放不一致

        if (physicalWidth <= 0 || physicalHeight <= 0)
            return 0.0;

        const double scaleX = static_cast<double>(physicalWidth) / logicalWidth;
        const double scaleY = static_cast<double>(physicalHeight) / logicalHeight;
        const double scale = (std::fabs(scaleX - scaleY) <= 0.05 * scaleX)
                                 ? (scaleX + scaleY) / 2.0
                                 : scaleX;
        LOG((CLOG_NOTE "kvm compositor: logical %dx%d, physical %dx%d, scale %.2f",
             logicalWidth, logicalHeight, physicalWidth, physicalHeight, scale));
        return sanitizeScale(scale);
    }

    static double scaleFromXftDpi(Display *display)
    {
        if (!display)
            return 0.0;

        char *resources = XResourceManagerString(display);
        if (!resources)
            return 0.0;

        XrmDatabase db = XrmGetStringDatabase(resources);
        if (!db)
            return 0.0;

        char *type = nullptr;
        XrmValue value{};
        double dpi = 0.0;
        if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value) && value.addr)
            dpi = atof(value.addr);
        XrmDestroyDatabase(db);

        if (dpi <= 96.0)
            return 0.0;

        return sanitizeScale(dpi / 96.0);
    }

    static double detectScaleFactor(Display *display, int physicalWidth, int physicalHeight)
    {
        // 允许外部显式指定，便于排查和特殊环境
        const char *env = getenv("DDE_COOPERATION_WAYLAND_SCALE");
        if (env && *env) {
            double scale = sanitizeScale(atof(env));
            LOG((CLOG_NOTE "kvm scale factor: %.2f (env)", scale));
            return scale;
        }

        const double compositor = scaleFromCompositor(physicalWidth, physicalHeight);
        if (compositor > 0.0) {
            LOG((CLOG_NOTE "kvm scale factor: %.2f (compositor)", compositor));
            return compositor;
        }
        if (compositor < 0.0) {
            // 各屏缩放不一致：没有统一比例，宁可不换算，也不回退 dpi
            LOG((CLOG_WARN "kvm: outputs use different scales, coordinate mapping disabled"));
            return 1.0;
        }

        const double scale = scaleFromXftDpi(display);
        if (scale > 0.0) {
            LOG((CLOG_NOTE "kvm scale factor: %.2f (Xft.dpi)", scale));
            return scale;
        }

        LOG((CLOG_NOTE "kvm scale factor: 1.00 (fallback)"));
        return 1.0;
    }
};

// 静态成员变量初始化
WaylandUtils::KvmPointerSetPosFunc WaylandUtils::s_kvmPointerSetPos = nullptr;
std::recursive_timed_mutex WaylandUtils::s_callbackMutex;
double WaylandUtils::s_scaleFactor = 1.0;
int WaylandUtils::s_physicalWidth = 0;
int WaylandUtils::s_physicalHeight = 0;
int WaylandUtils::s_logicalWidth = 0;
int WaylandUtils::s_logicalHeight = 0;
std::atomic<bool> WaylandUtils::s_active(false);
std::atomic<void *> WaylandUtils::s_screen(nullptr);
WaylandUtils::DestroyKvmFunc WaylandUtils::s_destroyKvm = nullptr;
std::atomic<int> WaylandUtils::s_inFlight(0);
std::mutex WaylandUtils::s_drainMutex;
std::condition_variable WaylandUtils::s_drainCond;
WaylandUtils::SyntheticMark WaylandUtils::s_synthMarks[WaylandUtils::kSyntheticMarkCount];
int WaylandUtils::s_synthMarkNext = 0;
double WaylandUtils::s_relAccumX = 0.0;
double WaylandUtils::s_relAccumY = 0.0;

#endif   // WAYLANDUTILS_H
