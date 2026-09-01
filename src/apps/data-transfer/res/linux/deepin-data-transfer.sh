#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later
# Deepin Data Transfer launcher
# 参照 deepin-home-appstore-client：满足条件时经 deepin-security-loader
# 附加 deepin-daemon 组启动，使 lastore 免鉴权接口调用可行；
# 否则直接启动，鉴权回退为系统 polkit 弹窗（等同历史行为）。

REAL_BIN=/usr/libexec/deepin/deepin-data-transfer

# Check if /usr/bin/deepin-security-loader is executable and dbus version meets
# requirement（用 -x 而非 -f：exec 失败时 bash 会直接退出 127，无法走 || 回退，
# 须在 exec 前排除不可执行的 loader）
if [ -x "/usr/bin/deepin-security-loader" ]; then
    # Check dbus version
    if DBUS_VERSION=$(dpkg-query -W -f='${Version}' dbus 2>/dev/null); then
        # Compare dbus version with 1.12.20.17-deepin1
        if dpkg --compare-versions "$DBUS_VERSION" ge "1.12.20.17-deepin1"; then
            # Use security loader with deepin-daemon group
            exec /usr/bin/deepin-security-loader --group deepin-daemon -- "$REAL_BIN" "$@"
        fi
    fi
fi

# Fallback to direct launch
exec "$REAL_BIN" "$@"
