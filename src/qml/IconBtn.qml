import QtQuick
import QtQuick.Controls

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
    radius: 8
    color: pressed ? Qt.rgba(1,1,1,0.18)
         : hovered ? Qt.rgba(1,1,1,0.10)
         : Qt.rgba(1,1,1,0.06)
    property bool pressed: false

    Behavior on color { ColorAnimation { duration: 110 } }

    opacity: enabled ? 1.0 : 0.35

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
        onEntered: root.hovered = true
        onExited:  { root.hovered = false; root.pressed = false }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: if (root.enabled) root.clicked()
    }
}
