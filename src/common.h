// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

enum class InputDeviceType : uint8_t {
    KEYBOARD = 1,
    MOUSE,
    TOUCHPAD,
};

#endif // !COMMON_H
