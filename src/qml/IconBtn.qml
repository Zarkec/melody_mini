import QtQuick
import QtQuick.Controls
import Melody

// Reusable icon button with hover/press feedback
Rectangle {
    id: root
    property string source: ""
    property string tooltip: ""
    property bool hovered: false
    signal clicked()

    width: 36; height: 36
    implicitWidth: width
    implicitHeight: height
    radius: Theme.radiusSm
    color: pressed ? Theme.overlay18
         : hovered ? Theme.overlay10
         : Theme.overlay06
    property bool pressed: false

    scale: pressed ? 0.96 : 1.0
    Behavior on color { ColorAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

    opacity: enabled ? 1.0 : 0.35
    Behavior on opacity { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }

    Image {
        anchors.centerIn: parent
        source: root.source
        width: parent.width * 0.52
        height: parent.height * 0.52
        smooth: true
        fillMode: Image.PreserveAspectFit
    }

    ToolTip.visible: root.tooltip !== "" && hovered
    ToolTip.text: root.tooltip
    ToolTip.delay: 600

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovered = true
        onExited:  { root.hovered = false; root.pressed = false }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: if (root.enabled) root.clicked()
    }
}
