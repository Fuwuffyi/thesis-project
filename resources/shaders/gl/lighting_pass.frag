#version 460

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D gAlbedoSampler;
layout(binding = 2) uniform sampler2D gNormalSampler;
layout(binding = 3) uniform sampler2D gDepthSampler;

void main() {
   fragColor = texture(gAlbedoSampler, fragUV) * texture(gNormalSampler, fragUV);
}
