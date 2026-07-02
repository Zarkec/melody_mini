import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// Always-visible mini player bar at the bottom of the window
Item {
    id: root
    signal tapped()

    Rectangle {
        id: bgRect
        anchors.fill: parent
        color: Qt.rgba(0.05, 0.05, 0.08, 0.92)
        radius: 8
        // Cover top corners to keep them square (only round bottom window corners)
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 8
            color: bgRect.color
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.tapped()
        }

        // Top border accent line
        Rectangle {
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.3; color: "#a78bfa" }
                GradientStop { position: 0.7; color: "#818cf8" }
                GradientStop { position: 1; color: "transparent" }
            }
        }

        RowLayout {
            anchors { fill: parent; leftMargin: 14; rightMargin: 14 }
            spacing: 10

            // Album art thumbnail
            Rectangle {
                width: 44; height: 44; radius: 8
                color: Qt.rgba(1,1,1,0.06)
                clip: true

                Image {
                    anchors.fill: parent
                    source: controller.albumArtUrl
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    asynchronous: true
                }
                Text {
                    visible: controller.albumArtUrl == ""
                    anchors.centerIn: parent
                    text: "♪"; color: Qt.rgba(1,1,1,0.25); font.pixelSize: 20
                }

                MouseArea { anchors.fill: parent; onClicked: root.tapped() }
            }

            // Song info
            Column {
                Layout.fillWidth: true
                spacing: 2
                clip: true

                Text {
                    width: parent.width
                    text: controller.currentSongName || "未在播放"
                    color: "white"
                    font { pixelSize: 13; bold: true; family: "Segoe UI" }
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: controller.currentArtist || ""
                    color: Qt.rgba(1,1,1,0.45)
                    font { pixelSize: 11; family: "Segoe UI" }
                    elide: Text.ElideRight
                }
            }

            // Playback controls
            RowLayout {
                spacing: 4

                IconBtn {
                    source: "qrc:/icons/previous.png"
                    width: 32; height: 32; radius: 6
                    enabled: !controller.loading
                    onClicked: controller.playPrev()
                }

                Item {
                    width: 36; height: 36

                    Canvas {
                        id: barLoadingSpinner
                        anchors.centerIn: parent
                        width: 24; height: 24
                        visible: controller.loading
                        onVisibleChanged: if (visible) requestPaint()

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            
                            // Faint background ring
                            ctx.beginPath()
                            ctx.arc(width/2, height/2, width/2 - 2.0, 0, 2*Math.PI)
                            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.12)
                            ctx.lineWidth = 2.0
                            ctx.stroke()
                            
                            // Active spinning arc
                            ctx.beginPath()
                            ctx.arc(width/2, height/2, width/2 - 2.0, 0, 1.5*Math.PI)
                            ctx.strokeStyle = "#a78bfa"
                            ctx.lineWidth = 2.0
                            ctx.stroke()
                        }

                        RotationAnimation on rotation {
                            from: 0; to: 360
                            duration: 900
                            loops: Animation.Infinite
                            running: barLoadingSpinner.visible
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: 18
                        visible: !controller.loading
                        color: Qt.rgba(1,1,1,0.10)

                        Image {
                            anchors.centerIn: parent
                            source: controller.playing ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                            width: 16; height: 16
                            smooth: true
                        }
                        MouseArea { anchors.fill: parent; onClicked: controller.playPause() }
                    }
                }

                IconBtn {
                    source: "qrc:/icons/next.png"
                    width: 32; height: 32; radius: 6
                    enabled: !controller.loading
                    onClicked: controller.playNext()
                }
            }
        }
    }
}
