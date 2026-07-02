import QtQuick
import QtQuick.Layouts

// Custom frameless title bar with drag support and window controls
Item {
    id: root

    signal closeRequested()
    signal minimizeRequested()
    signal dragStarted(point pos)
    signal dragging(point pos)
    signal dragEnded()

    // Drag area (whole bar)
    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => root.dragStarted(Qt.point(mouse.x, mouse.y))
        onPositionChanged: (mouse) => root.dragging(Qt.point(mouse.x, mouse.y))
        onReleased: root.dragEnded()
    }

    // App logo + title
    RowLayout {
        anchors { verticalCenter: parent.verticalCenter; left: parent.left; leftMargin: 14 }
        spacing: 8

        Image {
            source: "qrc:/logo.png"
            sourceSize.width: 22
            sourceSize.height: 22
            smooth: true
        }

        Text {
            text: "Melody"
            color: "white"
            font { pixelSize: 14; weight: Font.Medium }
            opacity: 0.9
        }
    }

    // Window control buttons
    RowLayout {
        anchors { verticalCenter: parent.verticalCenter; right: parent.right; rightMargin: 10 }
        spacing: 4

        // Minimize
        WinButton {
            symbol: "─"
            onClicked: root.minimizeRequested()
        }

        // Close
        WinButton {
            symbol: "✕"
            hoverColor: "#e74c3c"
            onClicked: root.closeRequested()
        }
    }

    // Bottom border
    Rectangle {
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: 1
        color: Qt.rgba(1,1,1,0.07)
    }
}
