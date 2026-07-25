#version 440

// Rounded-corner alpha mask for a whole item subtree (used as layer.effect).
// Unlike roundedimage.frag this does no cropping — it samples the layer
// texture as-is and only applies the rounded-rect alpha with a wide,
// finely-stepped AA fringe (~2px) so window corners look smooth at DPR 1,
// where Qt's built-in ~1px rectangle AA reads as staircase steps.

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float radius;
    vec2 size;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main()
{
    vec4 color = texture(source, qt_TexCoord0);

    // Signed distance to a rounded rect centered in the item.
    vec2 p = qt_TexCoord0 * ubuf.size;
    float r = min(ubuf.radius, min(ubuf.size.x, ubuf.size.y) * 0.5);
    vec2 q = abs(p - ubuf.size * 0.5) - (ubuf.size * 0.5 - vec2(r));
    float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
    float alpha = 1.0 - smoothstep(-1.0, 1.0, dist); // ~2px AA fringe
    fragColor = color * alpha * ubuf.qt_Opacity;
}
