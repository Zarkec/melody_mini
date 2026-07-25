import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import Melody

Window {
    id: island
    width: 340
    height: 52
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
            x = (Screen.width - width) / 2
            y = 10
            _dragging = false
        }
    }

    Rectangle {
        id: islandShell
        anchors.fill: parent
        radius: height / 2
        // Sleek semi-transparent dark background
        color: Qt.rgba(0.08, 0.08, 0.12, 0.92)
        border { color: Qt.rgba(1,1,1,0.1); width: 1 }

        // Widen the outer-edge AA fringe (~2px) so the pill edge looks
        // smooth at DPR 1; the shell keeps its own radius so the 1px
        // border follows the curve correctly.
        layer.enabled: true
        layer.smooth: true
        layer.effect: WindowCornerMask { radius: islandShell.height / 2 }

        // Fade/scale in on show so the handoff from main window feels intentional.
        scale: island.visible ? 1 : 0.96
        opacity: island.visible ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }

        RowLayout {
            anchors { fill: parent; leftMargin: Theme.spaceMd; rightMargin: Theme.spaceMd }
            spacing: Theme.spaceSm

            // Album art thumbnail
            Rectangle {
                width: 34; height: 34
                Layout.preferredWidth: 34
                Layout.preferredHeight: 34
                radius: 17
                color: Theme.overlay06
                Layout.alignment: Qt.AlignVCenter

                RoundedImage {
                    anchors.fill: parent
                    radius: parent.width / 2
                    source: controller.albumArtUrl
                }
                Text {
                    visible: controller.albumArtUrl == ""
                    anchors.centerIn: parent
                    text: "♪"; color: Qt.rgba(1,1,1,0.25)
                    font { pixelSize: 14; family: Theme.fontMain }
                }
            }

            // Song Info (Title & Artist)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.rightMargin: Theme.spaceSm
                Layout.alignment: Qt.AlignVCenter
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: controller.currentSongName || "未在播放"
                    color: Theme.textPrimary
                    font { pixelSize: 12; bold: true; family: Theme.fontMain }
                    elide: Text.ElideRight
                    // Color-emoji fallback fonts report a large ascent, which
                    // would otherwise stretch the line box and crowd the artist.
                    lineHeight: 0.95
                    lineHeightMode: Text.ProportionalHeight
                }
                Text {
                    Layout.fillWidth: true
                    text: controller.currentArtist || ""
                    color: Qt.rgba(1,1,1,0.6)
                    font { pixelSize: 10; family: Theme.fontMain }
                    elide: Text.ElideRight
                }
            }

            // Control Buttons
            RowLayout {
                spacing: 6
                Layout.alignment: Qt.AlignVCenter

                IconBtn {
                    source: "qrc:/icons/previous.png"
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    radius: 12
                    onClicked: controller.playPrev()
                }
                IconBtn {
                    source: controller.playing ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    radius: 12
                    onClicked: controller.playPause()
                }
                IconBtn {
                    source: "qrc:/icons/next.png"
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    radius: 12
                    onClicked: controller.playNext()
                }
            }
        }

        // Window drag & double-click area
        MouseArea {
            anchors.fill: parent
            z: -1 // Render behind control buttons
            cursorShape: Qt.ArrowCursor
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
