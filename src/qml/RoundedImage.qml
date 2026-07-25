import QtQuick

// Image with true rounded-corner clipping.
//
// Rectangle's clip:true only scissors to the rectangular bounds and ignores
// radius, so "rounded image" via a clipped Rectangle still shows square
// corners. Qt5Compat's OpacityMask would fix it but isn't always installed,
// so this uses a small rounded-rect SDF shader instead — no extra modules.
//
// Only fillMode PreserveAspectCrop is supported: the crop is computed in
// the shader because a hidden Image used as a texture source hands over
// its raw (uncropped) texture.
Item {
    id: root
    property alias source: img.source
    property alias asynchronous: img.asynchronous
    property alias status: img.status
    property real radius: 0

    implicitWidth: img.implicitWidth
    implicitHeight: img.implicitHeight

    Image {
        id: img
        anchors.fill: parent
        visible: false // only feeds the shader (as a texture provider)
        smooth: true
    }

    ShaderEffect {
        anchors.fill: parent
        visible: img.status === Image.Ready
        property variant source: img
        property real radius: root.radius
        property size size: Qt.size(width, height)
        property size imgSize: img.implicitWidth > 0
                               ? Qt.size(img.implicitWidth, img.implicitHeight)
                               : Qt.size(1, 1)

        // Qt 6 requires pre-baked .qsb shaders (see src/shaders/).
        fragmentShader: "qrc:/shaders/roundedimage.frag.qsb"
    }
}
