import QtQuick

// Apple-Music-style flowing radial-gradient background
// Animates smoothly between colour blobs
Item {
    id: root
    property var colors: ["#1a1a2e","#16213e","#0f3460"]

    // Time driver
    NumberAnimation on _time {
        from: 0; to: 100; duration: 22000
        loops: Animation.Infinite
        running: true
    }
    property real _time: 0
    onColorsChanged: canvas.requestPaint()
    on_TimeChanged:  canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            // Clip to rounded rect matching window corners
            ctx.beginPath()
            ctx.moveTo(8, 0)
            ctx.lineTo(width - 8, 0)
            ctx.arcTo(width, 0, width, 8, 8)
            ctx.lineTo(width, height - 8)
            ctx.arcTo(width, height, width - 8, height, 8)
            ctx.lineTo(8, height)
            ctx.arcTo(0, height, 0, height - 8, 8)
            ctx.lineTo(0, 8)
            ctx.arcTo(0, 0, 8, 0, 8)
            ctx.closePath()
            ctx.clip()

            // Base fill
            ctx.fillStyle = "#0d0d12"
            ctx.fillRect(0, 0, width, height)

            var t = root._time
            var cols = root.colors

            // Draw each colour as an animated radial blob
            for (var i = 0; i < cols.length; i++) {
                var phase = i * 2.1
                var cx = (0.25 + i * 0.3) * width + Math.sin(t * 0.06 + phase) * width * 0.22
                var cy = (0.3  + i * 0.25) * height + Math.cos(t * 0.05 + phase) * height * 0.20
                var r  = Math.max(width, height) * (0.52 + i * 0.12)

                var grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, r)
                var hex = cols[i] || "#333"
                // parse hex → r,g,b
                var ri = parseInt(hex.slice(1,3),16)
                var gi = parseInt(hex.slice(3,5),16)
                var bi = parseInt(hex.slice(5,7),16)
                grad.addColorStop(0,   "rgba(" + ri + "," + gi + "," + bi + ",0.70)")
                grad.addColorStop(0.5, "rgba(" + ri + "," + gi + "," + bi + ",0.28)")
                grad.addColorStop(1,   "rgba(" + ri + "," + gi + "," + bi + ",0.00)")
                ctx.fillStyle = grad
                ctx.fillRect(0, 0, width, height)
            }

            // Subtle dark vignette
            var vig = ctx.createRadialGradient(width/2, height/2, height*0.2, width/2, height/2, height*0.9)
            vig.addColorStop(0, "rgba(0,0,0,0)")
            vig.addColorStop(1, "rgba(0,0,0,0.45)")
            ctx.fillStyle = vig
            ctx.fillRect(0, 0, width, height)
        }
    }
}
