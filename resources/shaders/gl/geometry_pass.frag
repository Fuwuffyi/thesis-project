#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(location = 0) out vec4 gAlbedo; // RGB color + A AO
layout(location = 1) out vec4 gNormal; // RG encoded normal + B roughness + A metallic

layout(binding = 1) uniform sampler2D texSampler;

vec2 encodeOctNormal(vec3 n) {
   n /= (abs(n.x) + abs(n.y) + abs(n.z));
   vec2 enc = n.xy;
   if (n.z < 0.0) {
      enc = (1.0 - abs(enc.yx)) * sign(enc.xy);
   }
   return enc * 0.5 + 0.5;
}

void main() {
   gAlbedo = vec4(texture(texSampler, fragUV).rgb, 1.0);
   gNormal = vec4(encodeOctNormal(fragNormal), 1.0, 0.0);
}
