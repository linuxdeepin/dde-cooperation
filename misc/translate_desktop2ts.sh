#!/bin/bash

# SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

DESKTOP_SOURCE_FILE=config/compressor-singlefile.conf
DESKTOP_TS_DIR=../translations/config/

/usr/bin/deepin-desktop-ts-convert desktop2ts $DESKTOP_SOURCE_FILE $DESKTOP_TS_DIR
