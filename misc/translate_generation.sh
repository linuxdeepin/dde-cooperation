#!/bin/bash

# SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# this file is used to auto-generate .qm file from .ts file.
# author: shibowen at linuxdeepin.com

ts_list=(`ls translations/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lrelease "${ts}"
done
