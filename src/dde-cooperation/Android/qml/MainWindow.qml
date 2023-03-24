// SPDX-FileCopyrightText: 2018 - 2023 Deepin Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

import QtQuick 2.12
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import QtMultimedia 5.8

Window {
    id: window
    width: 640
    height: 480
    visible: backend.stage != 0

    Loader {
        id: loader
        focus: true
        anchors.fill: parent

        source: getSource()

        function getSource() {
            if (backend.stage == "1") {
                return "NewDevice.qml";
            }

            if (backend.stage == "6") {
                return "VideoPlayer.qml";
            }

            return "";
        }
    }
}

