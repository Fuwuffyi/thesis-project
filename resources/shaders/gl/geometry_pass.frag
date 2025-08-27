#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 gAlbedo;
layout(location = 1) out vec3 gNormal;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
   gAlbedo = texture(texSampler, fragUV);
   gNormal = fragNormal;
}
