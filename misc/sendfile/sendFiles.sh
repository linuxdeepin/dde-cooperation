#!/bin/bash

# SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

#echo $* > /tmp/tmp.txt

path=""
for var in $@
do
        path="$path$var"","
done
path=${path%?}

dbus-send --print-reply --dest=org.deepin.dde.Cooperation1 /org/deepin/dde/Cooperation1 org.deepin.dde.Cooperation1.SendFile array:string:$path int32:2
