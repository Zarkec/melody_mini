import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: island
    width: 300
    height: 46
    x: (Screen.width - width) / 2
    y: 10
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: false

    // Drag support
    property point _dragStart
    property bool _dragging: false

    signal restoreRequested()

    onVisibleChanged: {
        if (visible) {
            _dragging = false
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 23
        // Sleek semi-transparent dark background
        color: Qt.rgba(0.08, 0.08, 0.12, 0.92)
        border { color: Qt.rgba(1,1,1,0.1); width: 1 }

        RowLayout {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12 }
            spacing: 8

            // Album art thumbnail
            Rectangle {
                width: 32; height: 32
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32
                radius: 16
                clip: true
                color: Qt.rgba(1,1,1,0.06)
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.fill: parent
                    source: controller.albumArtUrl
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                }
                Text {
                    visible: controller.albumArtUrl == ""
                    anchors.centerIn: parent
                    text: "♪"; color: Qt.rgba(1,1,1,0.25); font.pixelSize: 14
                }
            }

            // Song Info (Title & Artist)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                spacing: 1

                Text {
                    Layout.fillWidth: true
                    text: controller.currentSongName || "未在播放"
                    color: "white"
                    font { pixelSize: 11; bold: true; family: "Segoe UI" }
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: controller.currentArtist || ""
                    color: Qt.rgba(1,1,1,0.6)
                    font { pixelSize: 9; family: "Segoe UI" }
                    elide: Text.ElideRight
                }
            }

            // Control Buttons
            RowLayout {
                spacing: 4
                Layout.alignment: Qt.AlignVCenter

                IconBtn {
                    source: "qrc:/icons/previous.png"
                    width: 22; height: 22; radius: 11
                    onClicked: controller.playPrev()
                }
                IconBtn {
                    source: controller.playing ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                    width: 22; height: 22; radius: 11
                    onClicked: controller.playPause()
                }
                IconBtn {
                    source: "qrc:/icons/next.png"
                    width: 22; height: 22; radius: 11
                    onClicked: controller.playNext()
                }
            }
        }

        // Window drag & double-click area
        MouseArea {
            anchors.fill: parent
            z: -1 // Render behind control buttons
            onPressed: (mouse) => {
                island._dragStart = Qt.point(mouse.x, mouse.y)
                island._dragging = true
            }
            onPositionChanged: (mouse) => {
                if (island._dragging) {
                    island.x += mouse.x - island._dragStart.x
                    island.y += mouse.y - island._dragStart.y
                }
            }
            onReleased: island._dragging = false
            onDoubleClicked: {
                island.restoreRequested()
            }
        }
    }
}
