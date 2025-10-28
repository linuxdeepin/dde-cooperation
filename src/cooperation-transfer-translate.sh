#!/bin/bash

# SPDX-FileCopyrightText: 2022 - 2023 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

ts_list=(`ls ../translations/dfmplugin-deliver/*.ts`)

for ts in "${ts_list[@]}"
do
    printf "\nprocess ${ts}\n"
    lupdate ./lib/cooperation/dfmplugin -ts -no-obsolete "${ts}"
done