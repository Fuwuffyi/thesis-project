#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 gAlbedo;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
   gAlbedo = mix(vec4(fragColor, 1.0), texture(texSampler, fragUV), 0.8);
}
