// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DISPLAYBASE_H
#define DISPLAYBASE_H

#include <vector>

#include <cstdint>

class Manager;

class DisplayBase {
public:
    explicit DisplayBase(Manager *manager);
    virtual ~DisplayBase() = default;

    void flowBack(uint16_t direction, uint16_t x, uint16_t y);
    void startEdgeDetection() { m_startEdgeDetection = true; };
    void stopEdgeDetection() { m_startEdgeDetection = false; };
    virtual void hideMouse(bool hide) = 0;

protected:
    virtual void moveMouse(uint16_t x, uint16_t y) = 0;

    bool edgeDetectionStarted() { return m_startEdgeDetection; }

    void handleScreenSizeChange(int16_t w, int16_t h);
    void handleMotion(int16_t x, int16_t y, bool evFromPeer);

private:
    Manager *m_manager;

    uint16_t m_screenWidth;
    uint16_t m_screenHeight;

    bool m_startEdgeDetection;

    uint16_t m_lastX;
    uint16_t m_lastY;

    void flowOut(uint16_t direction, uint16_t x, uint16_t y, bool evFromPeer);
};

#endif // !DISPLAYBASE_H
