// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include <dlfcn.h>

#include "lib/platform/waylandUtils.h"

// 访问 WaylandUtils 的内部纯函数。用 friend 而不是 #define private public：
// 后者会改变整个头文件（含标准库头）的访问控制，属于 ODR 隐患。
struct WaylandUtilsTestPeer {
    using OutputBox = WaylandUtils::OutputBox;
    using WlInterfaceDef = WaylandUtils::WlInterfaceDef;

    static int logicalToPhysical(double value, int logicalSize, int physicalSize)
    {
        return WaylandUtils::logicalToPhysical(value, logicalSize, physicalSize);
    }

    static double physicalToLogical(double value, int physicalSize, int logicalSize)
    {
        return WaylandUtils::physicalToLogical(value, physicalSize, logicalSize);
    }

    static double sanitizeScale(double scale) { return WaylandUtils::sanitizeScale(scale); }

    static void logicalSizeFor(int physicalWidth, int physicalHeight, double scale,
                               int &logicalWidth, int &logicalHeight)
    {
        WaylandUtils::logicalSizeFor(physicalWidth, physicalHeight, scale,
                                     logicalWidth, logicalHeight);
    }

    static double outputScaleFor(const OutputBox &box) { return WaylandUtils::outputScaleFor(box); }

    static double aggregateOutputs(const OutputBox *boxes, int count,
                                   int &logicalWidth, int &logicalHeight)
    {
        return WaylandUtils::aggregateOutputs(boxes, count, logicalWidth, logicalHeight);
    }

    static const WlInterfaceDef *expectedWlOutput() { return WaylandUtils::wlOutputInterfaceDef(); }

    static bool interfaceMatches(const WlInterfaceDef *expected, const void *realInterface)
    {
        return WaylandUtils::interfaceMatches(expected, realInterface);
    }

    static int wlOutputEventCount() { return WaylandUtils::kWlOutputEventCount; }

    static OutputBox makeBox(int32_t x, int32_t y, int32_t w, int32_t h,
                             int32_t physW = 0, int32_t physH = 0)
    {
        OutputBox box{};
        box.x = x;
        box.y = y;
        box.w = w;
        box.h = h;
        box.physW = physW;
        box.physH = physH;
        box.hasPosition = true;
        box.hasSize = true;
        box.hasMode = (physW > 0 && physH > 0);
        return box;
    }

    // 设置一套物理/逻辑尺寸，模拟不同缩放下 barrier 的坐标空间
    static void setScreen(double scale, int physicalWidth, int physicalHeight)
    {
        int logicalWidth = 0;
        int logicalHeight = 0;
        logicalSizeFor(physicalWidth, physicalHeight, scale, logicalWidth, logicalHeight);

        WaylandUtils::s_scaleFactor = scale;
        WaylandUtils::s_physicalWidth = physicalWidth;
        WaylandUtils::s_physicalHeight = physicalHeight;
        WaylandUtils::s_logicalWidth = logicalWidth;
        WaylandUtils::s_logicalHeight = logicalHeight;
        WaylandUtils::s_relAccumX = 0.0;
        WaylandUtils::s_relAccumY = 0.0;
    }

    static int logicalWidth() { return WaylandUtils::s_logicalWidth; }
    static int logicalHeight() { return WaylandUtils::s_logicalHeight; }
    static int physicalWidth() { return WaylandUtils::s_physicalWidth; }
};

// 逻辑边界必须精确落到物理边界，中点不能漂
TEST(WaylandKvmMapping, EdgeMapsToPhysicalBorder)
{
    struct Case {
        double scale;
        int width;
        int height;
    } cases[] = {
        { 1.0, 2160, 1440 },
        { 1.25, 2160, 1440 },
        { 1.5, 2880, 1920 },
        { 1.75, 2160, 1440 },
        { 2.0, 2160, 1440 },
    };

    for (const Case &c : cases) {
        WaylandUtilsTestPeer::setScreen(c.scale, c.width, c.height);
        const int logicalWidth = WaylandUtilsTestPeer::logicalWidth();
        const int logicalHeight = WaylandUtilsTestPeer::logicalHeight();

        EXPECT_EQ(WaylandUtilsTestPeer::logicalToPhysical(0, logicalWidth, c.width), 0);
        EXPECT_EQ(WaylandUtilsTestPeer::logicalToPhysical(logicalWidth - 1, logicalWidth, c.width),
                  c.width - 1);
        EXPECT_EQ(WaylandUtilsTestPeer::logicalToPhysical(logicalHeight - 1, logicalHeight, c.height),
                  c.height - 1);
        EXPECT_EQ(static_cast<int>(std::lround(WaylandUtilsTestPeer::physicalToLogical(
                      c.width - 1, c.width, logicalWidth))),
                  logicalWidth - 1);
    }
}

// 映射必须单调，不能出现回跳
TEST(WaylandKvmMapping, MappingIsMonotonic)
{
    WaylandUtilsTestPeer::setScreen(1.75, 2160, 1440);
    int previous = -1;
    for (int logical = 0; logical < WaylandUtilsTestPeer::logicalWidth(); ++logical) {
        const int physical = WaylandUtilsTestPeer::logicalToPhysical(
            logical, WaylandUtilsTestPeer::logicalWidth(), WaylandUtilsTestPeer::physicalWidth());
        EXPECT_GE(physical, previous);
        previous = physical;
    }
}

// 非整数缩放下逻辑尺寸按物理尺寸/缩放取整
TEST(WaylandKvmMapping, LogicalSizeRoundsPhysicalSize)
{
    int logicalWidth = 0;
    int logicalHeight = 0;

    WaylandUtilsTestPeer::logicalSizeFor(2880, 1920, 1.75, logicalWidth, logicalHeight);
    EXPECT_EQ(logicalWidth, 1646);    // 2880/1.75 = 1645.7
    EXPECT_EQ(logicalHeight, 1097);   // 1920/1.75 = 1097.1

    WaylandUtilsTestPeer::logicalSizeFor(0, 1920, 1.75, logicalWidth, logicalHeight);
    EXPECT_EQ(logicalWidth, 0);
    EXPECT_EQ(logicalHeight, 0);
}

// 相对位移按余数累加，多次小位移不能系统性少走
TEST(WaylandKvmRelativeMotion, RemainderIsCarriedOver)
{
    WaylandUtilsTestPeer::setScreen(1.75, 2160, 1440);
    int total = 0;
    for (int i = 0; i < 20; ++i) {
        SInt32 dx = 20;
        SInt32 dy = 0;
        WaylandUtils::physicalDeltaToDevice(dx, dy);
        total += dx;
    }
    // 20 次 × 20 物理像素 = 400，逻辑应为 400/1.75 ≈ 229
    EXPECT_NEAR(total, 229, 1);
}

// 回显命中一次即解除，不能把同位置的真实位移反复吞掉
TEST(WaylandSyntheticEcho, HitOnceThenReleased)
{
    WaylandUtils::markSyntheticPosition(100, 50);
    EXPECT_TRUE(WaylandUtils::isSyntheticEcho(101, 50));
    EXPECT_FALSE(WaylandUtils::isSyntheticEcho(101, 50));
}

// 距 warp 目标较远的移动本身不算回显；标记仍然保留，用于吞掉随后到达的那次回显
TEST(WaylandSyntheticEcho, FarMotionIsNotEcho)
{
    WaylandUtils::markSyntheticPosition(100, 50);
    EXPECT_FALSE(WaylandUtils::isSyntheticEcho(140, 50));
    EXPECT_TRUE(WaylandUtils::isSyntheticEcho(100, 50));
    EXPECT_FALSE(WaylandUtils::isSyntheticEcho(100, 50));
}

// 短时间内多次 warp 要各自独立记录（多槽）
TEST(WaylandSyntheticEcho, MultipleWarpsKeepSeparateMarks)
{
    WaylandUtils::markSyntheticPosition(100, 50);
    WaylandUtils::markSyntheticPosition(400, 300);
    EXPECT_TRUE(WaylandUtils::isSyntheticEcho(100, 50));
    EXPECT_TRUE(WaylandUtils::isSyntheticEcho(400, 300));
}

TEST(WaylandScaleSanitize, ClampsInvalidScale)
{
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::sanitizeScale(0.5), 1.0);
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::sanitizeScale(1.75), 1.75);
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::sanitizeScale(9.0), 1.0);
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::sanitizeScale(std::nan("")), 1.0);
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::sanitizeScale(HUGE_VAL), 1.0);
}

// 单屏：逻辑尺寸即该屏逻辑矩形，缩放比例一致
TEST(WaylandCompositorAggregate, SingleOutput)
{
    const WaylandUtilsTestPeer::OutputBox boxes[] = {
        WaylandUtilsTestPeer::makeBox(0, 0, 1646, 1097, 2880, 1920),
    };
    int logicalWidth = 0;
    int logicalHeight = 0;
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(boxes, 1, logicalWidth, logicalHeight),
                     1.0);
    EXPECT_EQ(logicalWidth, 1646);
    EXPECT_EQ(logicalHeight, 1097);
    EXPECT_NEAR(WaylandUtilsTestPeer::outputScaleFor(boxes[0]), 1.75, 0.01);
}

// 两屏缩放不同：明确返回 -1（不做换算），逻辑尺寸取并集
TEST(WaylandCompositorAggregate, MixedScaleIsRejected)
{
    const WaylandUtilsTestPeer::OutputBox boxes[] = {
        WaylandUtilsTestPeer::makeBox(0, 0, 1646, 1097, 2880, 1920),
        WaylandUtilsTestPeer::makeBox(1646, 0, 1920, 1080, 1920, 1080),
    };
    int logicalWidth = 0;
    int logicalHeight = 0;
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(boxes, 2, logicalWidth, logicalHeight),
                     -1.0);
    EXPECT_EQ(logicalWidth, 3566);
    EXPECT_EQ(logicalHeight, 1097);
}

// 旋转屏：mode 是未旋转的物理尺寸，逻辑矩形是旋转后的，按宽高对调仍算出同一缩放
TEST(WaylandCompositorAggregate, RotatedOutputKeepsScale)
{
    const WaylandUtilsTestPeer::OutputBox rotated =
        WaylandUtilsTestPeer::makeBox(0, 0, 1097, 1646, 2880, 1920);
    EXPECT_NEAR(WaylandUtilsTestPeer::outputScaleFor(rotated), 1.75, 0.01);

    // 一台横屏 + 一台竖屏（同一缩放）：不能误判成混合缩放
    const WaylandUtilsTestPeer::OutputBox boxes[] = {
        WaylandUtilsTestPeer::makeBox(0, 0, 1646, 1097, 2880, 1920),
        WaylandUtilsTestPeer::makeBox(1646, 0, 1097, 1646, 2880, 1920),
    };
    int logicalWidth = 0;
    int logicalHeight = 0;
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(boxes, 2, logicalWidth, logicalHeight),
                     1.0);
    EXPECT_EQ(logicalWidth, 2743);
    EXPECT_EQ(logicalHeight, 1646);
}

// 没有可用输出：返回 0（查询失败），让调用方回退 Xft.dpi
TEST(WaylandCompositorAggregate, NoUsableOutput)
{
    WaylandUtilsTestPeer::OutputBox noSize{};
    noSize.hasMode = false;
    int logicalWidth = 0;
    int logicalHeight = 0;
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(&noSize, 1, logicalWidth, logicalHeight),
                     0.0);
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(nullptr, 0, logicalWidth, logicalHeight),
                     0.0);
}

// 缺 mode 时算不出每屏缩放，但仍可用逻辑矩形（不判为混合）
TEST(WaylandCompositorAggregate, MissingModeStillUsable)
{
    const WaylandUtilsTestPeer::OutputBox boxes[] = {
        WaylandUtilsTestPeer::makeBox(0, 0, 1646, 1097),
    };
    int logicalWidth = 0;
    int logicalHeight = 0;
    EXPECT_DOUBLE_EQ(WaylandUtilsTestPeer::aggregateOutputs(boxes, 1, logicalWidth, logicalHeight),
                     1.0);
    EXPECT_EQ(WaylandUtilsTestPeer::outputScaleFor(boxes[0]), 0.0);
}

// 手写的 wl_output 事件表必须与 libwayland 导出的接口逐条一致
TEST(WaylandProtocolLayout, WlOutputEventsMatchLibrary)
{
    void *lib = dlopen("libwayland-client.so.0", RTLD_LAZY);
    if (!lib) {
        GTEST_LOG_(INFO) << "libwayland-client.so.0 not available, skip";
        return;
    }

    void *iface = dlsym(lib, "wl_output_interface");
    ASSERT_NE(nullptr, iface);
    EXPECT_EQ(WaylandUtilsTestPeer::expectedWlOutput()->eventCount,
              WaylandUtilsTestPeer::wlOutputEventCount());
    EXPECT_TRUE(WaylandUtilsTestPeer::interfaceMatches(
        WaylandUtilsTestPeer::expectedWlOutput(), iface));
    dlclose(lib);
}
