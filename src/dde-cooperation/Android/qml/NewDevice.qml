import QtQuick 2.12
import QtQuick.Window 2.15
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15
import QtMultimedia 5.8

ColumnLayout {
    spacing: 4

    Text {
        Layout.topMargin: 20
        Layout.alignment : Qt.AlignHCenter
        font.pixelSize: 14
        text: qsTr("Scan with UnionTech OS Assistant")
    }

    Text {
        Layout.alignment : Qt.AlignHCenter
        font.pixelSize: 11
        opacity: 0.6
        text: qsTr("Connect your phone to the computer via a USB cable for the first time")
    }

    Rectangle {
        Layout.topMargin: 20
        Layout.bottomMargin: 40
        Layout.leftMargin: 85
        Layout.rightMargin: 85
        width: 230
        height: 230
        radius: 18
        border.color: Qt.rgba(0,0,0,0.05)
        border.width: 11

        Canvas {
            id: canvas
            width: 190
            height: 190
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            antialiasing: true

            onPaint: {
                const s = qrCode.getSize();
                const aspect = width / height;
                const scale = ((aspect > 1.0) ? height : width) / s;

                var ctx = getContext("2d");
                ctx.save();
                ctx.reset();
                ctx.fillStyle = Qt.rgba(0, 0, 0, 1);

                for (let y = 0; y < s; y++) {
                    for (let x = 0; x < s; x++) {
                        if (qrCode.getModule(x, y)) {
                            const rx1 = x * scale;
                            const ry1 = y * scale;
                            ctx.fillRect(rx1, ry1, scale, scale);
                        }
                    }
                }

                ctx.restore();
            }

            Component.onCompleted: {
                qrCode.textChanged.connect(() => {
                    console.debug("update")
                    canvas.requestPaint()
                })
            }
        }
    }
}

