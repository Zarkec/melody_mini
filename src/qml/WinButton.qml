import QtQuick
import Melody

// Small window control button (minimize / close)
Rectangle {
    id: root
    width: 28; height: 28
    implicitWidth: width
    implicitHeight: height
    radius: Theme.radiusXs
    color: hovered ? (hoverColor !== "" ? hoverColor : Theme.overlay12) : "transparent"

    property string symbol: ""
    property string hoverColor: ""
    property bool hovered: false
    property bool pressed: false

    signal clicked()

    scale: pressed ? 0.94 : 1.0
    Behavior on color { ColorAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

    Text {
        anchors.centerIn: parent
        text: root.symbol
        color: Theme.textPrimary
        font { pixelSize: 18; family: Theme.fontMain }
        opacity: root.hovered ? 1.0 : 0.7
        Behavior on opacity { NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic } }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onEntered: root.hovered = true
        onExited:  { root.hovered = false; root.pressed = false }
        onPressed: root.pressed = true
        onReleased: root.pressed = false
        onClicked: root.clicked()
    }
}
