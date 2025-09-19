#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 gAlbedo; // RGB color + A AO
layout(location = 1) out vec4 gNormal; // RG encoded normal + B roughness + A metallic

void main() {
    gAlbedo = vec4(fragNormal, 1.0);
    gNormal = vec4(0.0, 1.0, 0.0, 1.0);
}
