import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform 1.1
import Melody

Window {
    id: mainWindow
    width: 380
    height: 620
    minimumWidth: 340
    minimumHeight: 560
    visible: true
    title: "Melody"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    // Hide to tray instead of quitting on close click
    property bool reallyQuit: false
    onClosing: (close) => {
        if (!reallyQuit) {
            close.accepted = false
            mainWindow.hide()
        }
    }

    // ── System Tray Icon ──────────────────────────────────────
    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: "qrc:/logo.png"
        tooltip: "Melody"

        menu: Menu {
            MenuItem {
                text: "显示窗口"
                onTriggered: {
                    mainWindow.show()
                    mainWindow.raise()
                    mainWindow.requestActivate()
                }
            }
            MenuItem {
                text: "退出"
                onTriggered: {
                    mainWindow.reallyQuit = true
                    Qt.quit()
                }
            }
        }

        onActivated: (reason) => {
            if (reason === SystemTrayIcon.DoubleClick || reason === SystemTrayIcon.Trigger) {
                mainWindow.show()
                mainWindow.raise()
                mainWindow.requestActivate()
            }
        }
    }

    // ── Floating Island ───────────────────────────────────────
    FloatingIsland {
        id: floatingIsland
        onRestoreRequested: {
            floatingIsland.hide()
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
        }
    }

    // Navigation helper
    function showPlayer() {
        console.log("QML: showPlayer() called. Stack depth:", stack.depth)
        if (stack.depth === 1)
            stack.push(playerPageComponent)
    }
    function showSearch() {
        if (stack.depth > 1)
            stack.pop()
    }

    // ── Drag to move ──────────────────────────────────────────
    property point _dragStart
    property bool _dragging: false

    // ── Page stack: 0 = search, 1 = player ────────────────────
    property int currentView: 0

    // ── Dynamic background colours from album art ─────────────
    property var bgColors: ["#1a1a2e","#16213e","#0f3460"]

    Connections {
        target: controller
        function onPaletteColorsChanged() {
            if (controller.paletteColors.length >= 1) {
                mainWindow.bgColors = controller.paletteColors
            }
        }
    }

    // ── Rounded window shell container ────────────────────────
    Rectangle {
        id: windowShell
        anchors.fill: parent
        // The outer rounded edge is produced by WindowCornerMask below
        // (wider, smoother AA than Rectangle's built-in ~1px fringe),
        // so the shell itself stays square.
        radius: 0
        color: Theme.surfaceBase
        clip: true

        layer.enabled: true
        layer.smooth: true
        layer.effect: WindowCornerMask { radius: Theme.radiusMd }

        // Spotify-style static gradient background
        Rectangle {
            id: spotifyBg
            anchors.fill: parent
            radius: Theme.radiusMd
            color: Theme.surfaceBase

            // Gradient fading from the dominant cover color to transparent
            Rectangle {
                anchors.fill: parent
                radius: Theme.radiusMd
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop {
                        position: 0.0
                        color: Qt.darker(mainWindow.bgColors.length >= 1 ? mainWindow.bgColors[0] : "#1e1e24", 1.2)
                    }
                    GradientStop {
                        position: 0.6
                        color: "transparent"
                    }
                }
            }
        }

        // ── Custom title bar ──────────────────────────────────────
        TitleBar {
            id: titleBar
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 44
            onCloseRequested:   mainWindow.close()
            onMinimizeRequested: mainWindow.showMinimized()
            onDragStarted: (pos) => {
                mainWindow._dragStart = pos
                mainWindow._dragging = true
            }
            onDragging: (pos) => {
                if (mainWindow._dragging) {
                    mainWindow.x += pos.x - mainWindow._dragStart.x
                    mainWindow.y += pos.y - mainWindow._dragStart.y
                }
            }
            onDragEnded: {
                mainWindow._dragging = false
            }
        }

        // ── PlayerPage Component to connect signals ──────────────
        Component {
            id: playerPageComponent
            PlayerPage {
                onMinimizeRequested: {
                    mainWindow.hide()
                    floatingIsland.show()
                }
            }
        }

        // ── Main content area ─────────────────────────────────────
        StackView {
            id: stack
            anchors {
                top: titleBar.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            clip: true
            initialItem: SearchPage {
                id: searchPage
                onSongClicked: (index) => {
                    console.log("QML: onSongClicked for index", index)
                    controller.playSongAt(index)
                    mainWindow.showPlayer()
                }
            }

            // Symmetric push/pop: both directions slide + fade, mirrored.
            pushEnter: Transition {
                ParallelAnimation {
                    PropertyAnimation { property: "x"; from: stack.width; to: 0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                    PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                }
            }
            pushExit: Transition {
                ParallelAnimation {
                    PropertyAnimation { property: "x"; from: 0; to: -stack.width * 0.3; duration: Theme.durPage; easing.type: Easing.OutCubic }
                    PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                }
            }
            popEnter: Transition {
                ParallelAnimation {
                    PropertyAnimation { property: "x"; from: -stack.width * 0.3; to: 0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                    PropertyAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                }
            }
            popExit: Transition {
                ParallelAnimation {
                    PropertyAnimation { property: "x"; from: 0; to: stack.width; duration: Theme.durPage; easing.type: Easing.OutCubic }
                    PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.durPage; easing.type: Easing.OutCubic }
                }
            }
        }

        // Mini player bar 的 tapped() 才是进入播放页的唯一入口；
        // 切歌（上一曲/下一曲/自动续播）不应强制跳转到播放页。
        // ── Mini player bar (always visible) ─────────────────────
        PlayerBar {
            id: playerBar
            anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
            height: 72
            // Fade instead of hard visibility cut during page navigation.
            opacity: stack.depth === 1 ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
            onTapped: {
                if (stack.depth === 1) mainWindow.showPlayer()
            }
        }
    }
}
