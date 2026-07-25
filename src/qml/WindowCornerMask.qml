import QtQuick
import Melody

// Rounded-corner mask for use as Item.layer.effect.
// When assigned to layer.effect, Qt automatically feeds the layer texture
// into the `source` property. See src/shaders/roundedmask.frag.
ShaderEffect {
    property variant source: null // assigned by the layer
    property real radius: Theme.radiusMd
    property size size: Qt.size(width, height)

    fragmentShader: "qrc:/shaders/roundedmask.frag.qsb"
}
