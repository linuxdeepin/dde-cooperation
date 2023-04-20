import QtQuick 2.12
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import QtMultimedia 5.8

Window {
    id: window
    title: " "
    width: loader.width
    height: loader.height
    visible: loader.status == Loader.Ready

    Loader {
        id: loader
        focus: true
        anchors.centerIn: parent

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

