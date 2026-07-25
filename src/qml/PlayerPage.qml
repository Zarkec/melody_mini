import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Melody

// Player page: album art, song info, lyrics, controls
Item {
    id: root
    signal minimizeRequested()

    ColumnLayout {
        anchors { fill: parent; margins: Theme.spaceLg; topMargin: 10 }
        spacing: Theme.spaceMd

        // Back button row
        RowLayout {
            Layout.fillWidth: true
            IconBtn {
                source: "qrc:/icons/back.png"
                width: 32; height: 32; radius: Theme.radiusSm
                onClicked: root.StackView.view.pop()
            }
            Item { Layout.fillWidth: true }
            // Bilibili page selector
            ComboBox {
                id: pageSelector
                visible: controller.bilibiliPages.length > 1
                model: controller.bilibiliPages
                textRole: "label"
                currentIndex: controller.currentBilibiliPage
                onActivated: (index) => controller.selectBilibiliPage(index)

                implicitWidth: 150
                implicitHeight: 32

                background: Rectangle {
                    color: Qt.rgba(1,1,1,0.08)
                    border.color: pageSelector.hovered ? Qt.rgba(1,1,1,0.18) : Qt.rgba(1,1,1,0.08)
                    border.width: 1
                    radius: Theme.radiusSm
                }

                contentItem: Text {
                    leftPadding: 10
                    rightPadding: 24
                    text: pageSelector.displayText
                    color: Theme.textPrimary
                    font { pixelSize: 11; family: Theme.fontMain }
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                indicator: Canvas {
                    id: canvas
                    x: pageSelector.width - width - 10
                    y: pageSelector.topPadding + (pageSelector.availableHeight - height) / 2
                    width: 8
                    height: 5
                    contextType: "2d"

                    Connections {
                        target: pageSelector
                        function onPressedChanged() { canvas.requestPaint() }
                    }

                    onPaint: {
                        var context = getContext("2d");
                        context.reset();
                        context.moveTo(0, 0);
                        context.lineTo(width, 0);
                        context.lineTo(width / 2, height);
                        context.closePath();
                        context.fillStyle = "white";
                        context.fill();
                    }
                }

                popup: Popup {
                    y: pageSelector.height + 4
                    width: pageSelector.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durSlow; easing.type: Easing.OutCubic } }
                    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durFast; easing.type: Easing.OutCubic } }

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight > 200 ? 200 : contentHeight
                        model: pageSelector.popup.visible ? pageSelector.delegateModel : null
                        currentIndex: pageSelector.highlightedIndex

                        ScrollIndicator.vertical: ScrollIndicator { }
                    }

                    background: Rectangle {
                        color: Theme.surfacePopup
                        border.color: Qt.rgba(1,1,1,0.1)
                        radius: Theme.radiusSm
                    }
                }

                delegate: ItemDelegate {
                    width: pageSelector.width
                    height: 28
                    contentItem: Text {
                        text: modelData.label || ""
                        color: highlighted ? Theme.textPrimary : Theme.textTertiary
                        font { pixelSize: 11; family: Theme.fontMain }
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: highlighted ? Theme.overlay10 : "transparent"
                        radius: Theme.radiusXs
                    }
                    padding: 6
                }
            }
        }

        // Album art (round corners, drop-shadow effect)
        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 200
            Layout.preferredHeight: 200

            Rectangle {
                id: coverRect
                anchors.fill: parent
                radius: Theme.radiusMd
                color: Theme.overlay06
                clip: true

                // Double-image crossfade on cover change. The dead
                // "Behavior on source" cannot crossfade; this does.
                RoundedImage {
                    id: artImgA
                    anchors.fill: parent
                    radius: Theme.radiusMd
                    asynchronous: true
                    opacity: 1
                    Behavior on opacity { NumberAnimation { duration: Theme.durPage; easing.type: Easing.OutCubic } }
                }
                RoundedImage {
                    id: artImgB
                    anchors.fill: parent
                    radius: Theme.radiusMd
                    asynchronous: true
                    opacity: 0
                    Behavior on opacity { NumberAnimation { duration: Theme.durPage; easing.type: Easing.OutCubic } }
                }

                property bool _useA: true
                function updateCover(url) {
                    if (_useA) {
                        artImgA.source = url
                        artImgA.opacity = 1
                        artImgB.opacity = 0
                    } else {
                        artImgB.source = url
                        artImgB.opacity = 1
                        artImgA.opacity = 0
                    }
                    _useA = !_useA
                }

                Component.onCompleted: coverRect.updateCover(controller.albumArtUrl)
                Connections {
                    target: controller
                    function onAlbumArtChanged() {
                        coverRect.updateCover(controller.albumArtUrl)
                    }
                }

                // Placeholder when no cover
                Column {
                    anchors.centerIn: parent
                    spacing: Theme.spaceSm
                    visible: controller.albumArtUrl == ""
                    Text {
                        text: "♪"
                        font { pixelSize: 50; family: Theme.fontMain }
                        color: Qt.rgba(1,1,1,0.3)
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Text {
                        text: "暂无封面"
                        font { pixelSize: 12; family: Theme.fontMain }
                        color: Qt.rgba(1,1,1,0.25)
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }

        // Song title + artist
        Column {
            Layout.fillWidth: true
            spacing: 4

            Text {
                width: parent.width
                text: controller.currentSongName || "未在播放"
                color: Theme.textPrimary
                font { pixelSize: 18; bold: true; family: Theme.fontMain }
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }
            Text {
                width: parent.width
                text: controller.currentArtist || ""
                color: Theme.textSecondary
                font { pixelSize: 13; family: Theme.fontMain }
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Lyric
        Text {
            Layout.fillWidth: true
            text: controller.currentLyric || "欢迎使用 Melody"
            color: Qt.rgba(1,1,1,0.60)
            font { pixelSize: 13; family: Theme.fontMain }
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.maximumHeight: 40
            clip: true

            Behavior on text {
                SequentialAnimation {
                    PropertyAnimation { target: lyricFade; property: "opacity"; to: 0; duration: 120 }
                    PropertyAction {}
                    PropertyAnimation { target: lyricFade; property: "opacity"; to: 1; duration: 200 }
                }
            }
            id: lyricFade
        }

        // Progress slider + time labels
        Column {
            Layout.fillWidth: true
            spacing: 4

            Slider {
                id: progressSlider
                width: parent.width
                value: controller.duration > 0 ? controller.position / controller.duration : 0
                enabled: controller.duration > 0
                onMoved: controller.seekTo(value * controller.duration)

                background: Rectangle {
                    x: progressSlider.leftPadding
                    y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                    width: progressSlider.availableWidth
                    height: 4
                    radius: 2
                    color: Qt.rgba(1,1,1,0.18)

                    Rectangle {
                        width: progressSlider.visualPosition * parent.width
                        height: parent.height
                        radius: 2
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: Theme.accentLight }
                            GradientStop { position: 1; color: Theme.accentDark }
                        }
                    }
                }

                handle: Rectangle {
                    x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                    y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                    width: 14; height: 14
                    radius: 7
                    color: Theme.textPrimary
                    opacity: progressSlider.pressed || progressSlider.hovered ? 1 : 0.85
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }
            }

            RowLayout {
                width: parent.width
                Text { text: controller.formatTime(controller.position); color: Qt.rgba(1,1,1,0.5); font { pixelSize: 11; family: Theme.fontMain } }
                Item { Layout.fillWidth: true }
                Text { text: controller.formatTime(controller.duration); color: Qt.rgba(1,1,1,0.5); font { pixelSize: 11; family: Theme.fontMain } }
            }
        }

        // Playback controls row (symmetrically centered)
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            spacing: 0

            // Left side wrapper (width: 80)
            Item {
                width: 80
                height: 56
                Layout.preferredWidth: 80
                Layout.preferredHeight: 56

                // Play mode
                IconBtn {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    source: {
                        if (controller.playMode === 1) return "qrc:/icons/loop-one.png"
                        if (controller.playMode === 2) return "qrc:/icons/shuffle.png"
                        return "qrc:/icons/loop-list.png"
                    }
                    width: 36; height: 36; radius: Theme.radiusSm
                    tooltip: ["顺序","单曲循环","随机"][controller.playMode] || ""
                    onClicked: controller.cyclePlayMode()
                }
            }

            Item { Layout.fillWidth: true } // Left expanding spacer

            // Middle playback control group
            RowLayout {
                spacing: Theme.spaceMd
                Layout.alignment: Qt.AlignHCenter

                IconBtn {
                    source: "qrc:/icons/previous.png"
                    width: 40; height: 40; radius: 10
                    enabled: !controller.loading
                    onClicked: controller.playPrev()
                }

                // Play/pause — bigger central button
                Item {
                    width: 56; height: 56
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56

                    Rectangle {
                        id: bigPlayBtn
                        anchors.fill: parent
                        radius: 28
                        property bool hovered: false
                        property bool pressed: false
                        color: pressed ? Theme.overlay18
                             : hovered ? Theme.overlay12
                             : Qt.rgba(1,1,1,0.15)
                        scale: pressed ? 0.96 : 1.0
                        Behavior on color { ColorAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
                        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

                        // Premium native QML loader (with transparent background)
                        Canvas {
                            id: loadingSpinner
                            anchors.centerIn: parent
                            width: 28; height: 28
                            visible: controller.loading
                            onVisibleChanged: if (visible) requestPaint()

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)

                                // Faint background ring
                                ctx.beginPath()
                                ctx.arc(width/2, height/2, width/2 - 2.5, 0, 2*Math.PI)
                                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.12)
                                ctx.lineWidth = 2.5
                                ctx.stroke()

                                // Active spinning arc
                                ctx.beginPath()
                                ctx.arc(width/2, height/2, width/2 - 2.5, 0, 1.5*Math.PI)
                                ctx.strokeStyle = Theme.accentLight
                                ctx.lineWidth = 2.5
                                ctx.stroke()
                            }

                            RotationAnimation on rotation {
                                from: 0; to: 360
                                duration: 900
                                loops: Animation.Infinite
                                running: loadingSpinner.visible
                            }
                        }

                        Image {
                            anchors.centerIn: parent
                            source: controller.playing ? "qrc:/icons/pause.png" : "qrc:/icons/play.png"
                            width: 22; height: 22
                            visible: !controller.loading
                            smooth: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onEntered: bigPlayBtn.hovered = true
                            onExited: { bigPlayBtn.hovered = false; bigPlayBtn.pressed = false }
                            onPressed: bigPlayBtn.pressed = true
                            onReleased: bigPlayBtn.pressed = false
                            onClicked: controller.playPause()
                        }
                    }
                }

                IconBtn {
                    source: "qrc:/icons/next.png"
                    width: 40; height: 40; radius: 10
                    enabled: !controller.loading
                    onClicked: controller.playNext()
                }
            }

            Item { Layout.fillWidth: true } // Right expanding spacer

            // Right side wrapper (width: 80)
            Item {
                width: 80
                height: 56
                Layout.preferredWidth: 80
                Layout.preferredHeight: 56

                Row {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: Theme.spaceSm

                    // Mini mode button
                    IconBtn {
                        source: "qrc:/icons/minimize.png"
                        width: 36; height: 36; radius: Theme.radiusSm
                        tooltip: "迷你模式"
                        onClicked: root.minimizeRequested()
                    }

                    // Volume button
                    IconBtn {
                        id: volBtn
                        source: {
                            var v = controller.volume
                            if (v === 0) return "qrc:/icons/volume-mute.png"
                            if (v < 30)  return "qrc:/icons/volume-low.png"
                            if (v < 70)  return "qrc:/icons/volume-medium.png"
                            return "qrc:/icons/volume-high.png"
                        }
                        width: 36; height: 36; radius: Theme.radiusSm
                        onClicked: volumePopup.visible = !volumePopup.visible
                    }
                }
            }
        }
    }

    // ── Volume + device popup ─────────────────────────────────
    Rectangle {
        id: volumePopup
        visible: false
        width: 220; height: 180
        radius: 14
        color: Qt.rgba(0.1, 0.1, 0.12, 0.96)
        border { color: Qt.rgba(1,1,1,0.08); width: 1 }
        anchors { right: parent.right; rightMargin: 14; bottom: parent.bottom; bottomMargin: 80 }
        z: 10

        // Fade in/out instead of instant visibility toggle.
        opacity: visible ? 1 : 0
        scale: visible ? 1 : 0.98
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }

        // Swallow mouse clicks to prevent closing the popup, placed behind Column
        MouseArea {
            anchors.fill: parent
        }

        Column {
            anchors { fill: parent; margins: Theme.spaceMd }
            spacing: 10

            Text {
                text: "输出设备"
                color: Qt.rgba(1,1,1,0.5)
                font { pixelSize: 10; family: Theme.fontMain }
            }

            ListView {
                width: parent.width
                height: 80
                model: controller.audioDevices
                clip: true
                delegate: Text {
                    width: parent ? parent.width : 0
                    text: modelData.name || ""
                    color: index === controller.currentAudioDeviceIndex ? Theme.textPrimary : Qt.rgba(1,1,1,0.45)
                    font { pixelSize: 11; bold: index === controller.currentAudioDeviceIndex; family: Theme.fontMain }
                    elide: Text.ElideRight
                    height: 22
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: controller.selectAudioDevice(index) }
                }
            }

            Text {
                text: "音量 " + controller.volume + "%"
                color: Qt.rgba(1,1,1,0.5)
                font { pixelSize: 10; family: Theme.fontMain }
            }

            Slider {
                width: parent.width
                value: controller.volume / 100
                onMoved: controller.volume = Math.round(value * 100)

                background: Rectangle {
                    x: parent.leftPadding; y: parent.topPadding + parent.availableHeight/2 - height/2
                    width: parent.availableWidth; height: 4; radius: 2
                    color: Qt.rgba(1,1,1,0.18)
                    Rectangle {
                        width: parent.parent.visualPosition * parent.width
                        height: parent.height; radius: 2; color: Theme.accentLight
                    }
                }
                handle: Rectangle {
                    x: parent.leftPadding + parent.visualPosition*(parent.availableWidth-width)
                    y: parent.topPadding + parent.availableHeight/2 - height/2
                    width: 14; height: 14; radius: 7; color: Theme.textPrimary
                }
            }
        }
    }

    // Close popup on click elsewhere
    MouseArea {
        anchors.fill: parent
        enabled: volumePopup.visible
        z: 9
        onClicked: volumePopup.visible = false
    }
}
