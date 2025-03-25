#ifndef WAYLANDUTILS_H
#define WAYLANDUTILS_H

#include "XWindowsScreen.h"
#include "base/Log.h"
#include <dlfcn.h>  // 用于动态加载库

class WaylandUtils
{
public:
    // 定义函数指针类型
    typedef int (*InitKvmFunc)();
    typedef void (*KvmRegisterPointerMotionFunc)(void*, void (*)(void*, unsigned int, double, double));
    typedef void (*KvmPointerSetPosFunc)(double, double);

    // 静态成员变量，用于存储函数指针
    static KvmPointerSetPosFunc s_kvmPointerSetPos;

    // 初始化动态库和函数指针，并处理回调
    static bool initWaylandKvm(void *user_data)
    {
        void* kvmLibHandle = dlopen("libdisplayjack_kvm.so", RTLD_LAZY);
        if (!kvmLibHandle) {
            LOG((CLOG_NOTE "Failed to load libdisplayjack_kvm.so: %s", dlerror()));
            return false;
        }

        InitKvmFunc initKvm = (InitKvmFunc)dlsym(kvmLibHandle, "init_kvm");
        KvmRegisterPointerMotionFunc kvmRegisterPointerMotion = (KvmRegisterPointerMotionFunc)dlsym(kvmLibHandle, "kvm_register_pointer_motion");
        s_kvmPointerSetPos = (KvmPointerSetPosFunc)dlsym(kvmLibHandle, "kvm_pointer_set_pos");

        if (!initKvm || !kvmRegisterPointerMotion || !s_kvmPointerSetPos) {
            LOG((CLOG_NOTE "Failed to resolve symbols: %s", dlerror()));
            dlclose(kvmLibHandle);
            return false;
        }

        int initSucceed = initKvm();
        LOG((CLOG_NOTE " init kvm status: %d", initSucceed));

        if (initSucceed == 0) {
            kvmRegisterPointerMotion(user_data, WaylandUtils::kvmPointerMotionCallback);
        }

        return true;
    }

    static void kvmPointerMotionCallback(void *user_data, unsigned int time, double x, double y)
    {
        XWindowsScreen *xsreen = static_cast<XWindowsScreen *>(user_data);
        if (!xsreen)
            return;
        XMotionEvent xmotion;
        xmotion.type = MotionNotify;
        xmotion.send_event = False;   // Raw motion
        xmotion.x_root = x;
        xmotion.y_root = y;
        xmotion.x = x;
        xmotion.y = y;
        xsreen->handleMouseMove(xmotion);
    }

    static bool setPointerPos(double x, double y)
    {
        if (s_kvmPointerSetPos) {
            s_kvmPointerSetPos(x, y);
        } else {
            // LOG((CLOG_WARN "kvm_pointer_set_pos is not available"));
            return false;
        }
        return true;
    }

    static bool isWayland()
    {
        char *type = getenv("XDG_SESSION_TYPE");
        char *display = getenv("WAYLAND_DISPLAY");

        return (type && strcmp(type, "wayland") == 0) || (display && strcmp(display, "wayland") == 0);
    }
};

// 静态成员变量初始化
WaylandUtils::KvmPointerSetPosFunc WaylandUtils::s_kvmPointerSetPos = nullptr;

#endif   // WAYLANDUTILS_H
