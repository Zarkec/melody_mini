import QtQuick

// Small window control button (minimize / close)
Rectangle {
    id: root
    width: 28; height: 28
    implicitWidth: width
    implicitHeight: height
    radius: 6
    color: hovered ? (hoverColor !== "" ? hoverColor : Qt.rgba(1,1,1,0.12)) : "transparent"

    property string symbol: ""
    property string hoverColor: ""
    property bool hovered: false

    signal clicked()

    Behavior on color { ColorAnimation { duration: 120 } }

    Text {
        anchors.centerIn: parent
        text: root.symbol
        color: "white"
        font.pixelSize: 22
        opacity: root.hovered ? 1.0 : 0.7
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onEntered: root.hovered = true
        onExited:  root.hovered = false
        onClicked: root.clicked()
    }
}
