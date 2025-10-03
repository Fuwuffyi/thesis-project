#version 460

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 coord = fragTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    if (dist > 1.0)
        discard;
    float alpha = 1.0 - smoothstep(0.7, 1.0, dist);
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
