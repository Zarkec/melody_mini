#version 440

// Rounded-corner image shader for RoundedImage.qml.
// Compiled to .qsb at build time via qt_add_shaders (Qt 6 requires
// pre-baked shaders; inline shader strings are not supported).

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float radius;
    vec2 size;
    vec2 imgSize;
} ubuf;

layout(binding = 1) uniform sampler2D source;

void main()
{
    // PreserveAspectCrop in UV space: sample only the centered fraction
    // of the texture that fills the item.
    float itemAspect = ubuf.size.x / ubuf.size.y;
    float imgAspect = ubuf.imgSize.x / ubuf.imgSize.y;
    vec2 uvScale = itemAspect < imgAspect
                 ? vec2(itemAspect / imgAspect, 1.0)
                 : vec2(1.0, imgAspect / itemAspect);
    vec2 uv = (qt_TexCoord0 - 0.5) * uvScale + 0.5;
    vec4 color = texture(source, uv);

    // Signed distance to a rounded rect centered in the item.
    vec2 p = qt_TexCoord0 * ubuf.size;
    float r = min(ubuf.radius, min(ubuf.size.x, ubuf.size.y) * 0.5);
    vec2 q = abs(p - ubuf.size * 0.5) - (ubuf.size * 0.5 - vec2(r));
    float dist = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
    float alpha = 1.0 - smoothstep(-0.75, 0.75, dist); // ~1.5px AA edge
    fragColor = color * alpha * ubuf.qt_Opacity;
}
