// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DisplayBase.h"

#include "Manager.h"
#include "protocol/device_sharing.pb.h"

DisplayBase::DisplayBase(Manager *manager)
    : m_manager(manager)
    , m_startEdgeDetection(false) {
}

void DisplayBase::handleScreenSizeChange(int16_t w, int16_t h) {
    m_screenWidth = w;
    m_screenHeight = h;
}

void DisplayBase::handleMotion(int16_t x, int16_t y, bool evFromPeer) {
    do {
        if (m_lastX == x) {
            if (x == 0) {
                // left flow
                qInfo("left flow");
                flowOut(FLOW_DIRECTION_LEFT, 0, y, evFromPeer);
                break;
            }

            if (x == m_screenWidth - 1) {
                // right flow
                qInfo("right flow");
                flowOut(FLOW_DIRECTION_RIGHT, 0, y, evFromPeer);
                break;
            }
        }

        if (m_lastY == y) {
            if (y == 0) {
                // top flow
                qInfo("top flow");
                flowOut(FLOW_DIRECTION_TOP, x, 0, evFromPeer);
                break;
            }

            if (y == m_screenHeight - 1) {
                // bottom flow
                qInfo("bottom flow");
                flowOut(FLOW_DIRECTION_BOTTOM, x, 0, evFromPeer);
                break;
            }
        }
    } while (false);

    m_lastX = x;
    m_lastY = y;
}

void DisplayBase::flowBack(uint16_t direction, uint16_t x, uint16_t y) {
    switch (direction) {
    case 3: {
        // left
        x = m_screenWidth;
        break;
    }
    }

    moveMouse(x, y);
    hideMouse(false);
    startEdgeDetection();
}

void DisplayBase::flowOut(uint16_t direction, uint16_t x, uint16_t y, bool evFromPeer) {
    bool r = m_manager->tryFlowOut(direction, x, y, evFromPeer);
    if (r) {
        // stopEdgeDetection();
        hideMouse(true);
    }
}
