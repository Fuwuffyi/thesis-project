#version 460

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D gAlbedoSampler;
layout(binding = 2) uniform sampler2D gNormalSampler;
layout(binding = 3) uniform sampler2D gDepthSampler;

vec3 decodeOctNormal(vec2 enc) {
    enc = enc * 2.0 - 1.0;
    vec3 n = vec3(enc.xy, 1.0 - abs(enc.x) - abs(enc.y));
    if (n.z < 0.0) {
        n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
    }
    return normalize(n);
}

void main() {
   fragColor = texture(gAlbedoSampler, fragUV) * vec4(decodeOctNormal(texture(gNormalSampler, fragUV).xy), 1.0);
}
