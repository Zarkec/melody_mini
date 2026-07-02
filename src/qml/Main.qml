import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform 1.1

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
        anchors.fill: parent
        radius: 8
        color: "#0d0d12"
        clip: true

        // ── Animated flowing background ───────────────────────────
        FlowingBackground {
            id: flowBg
            anchors.fill: parent
            colors: mainWindow.bgColors
        }

    // Dark overlay for readability
    Rectangle {
        anchors.fill: parent
        radius: 8
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Qt.rgba(0,0,0,0.55) }
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

        pushEnter: Transition {
            PropertyAnimation { property: "x"; from: stack.width; to: 0; duration: 260; easing.type: Easing.OutCubic }
        }
        pushExit: Transition {
            PropertyAnimation { property: "x"; from: 0; to: -stack.width * 0.3; duration: 260; easing.type: Easing.OutCubic }
        }
        popEnter: Transition {
            PropertyAnimation { property: "x"; from: -stack.width * 0.3; to: 0; duration: 260; easing.type: Easing.OutCubic }
        }
        popExit: Transition {
            ParallelAnimation {
                PropertyAnimation { property: "x"; from: 0; to: stack.width; duration: 260; easing.type: Easing.OutCubic }
                PropertyAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 260; easing.type: Easing.OutCubic }
            }
        }
    }



    Connections {
        target: controller
        function onCurrentSongChanged() {
            if (controller.currentSongName !== "") {
                mainWindow.showPlayer()
            }
        }
    }

    // ── Mini player bar (always visible) ─────────────────────
    PlayerBar {
        id: playerBar
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 72
        visible: stack.depth === 1
        onTapped: {
            if (stack.depth === 1) mainWindow.showPlayer()
        }
    }
}
}
