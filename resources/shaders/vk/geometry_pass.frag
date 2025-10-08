#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(set = 0, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
} camera;

layout(set = 1, binding = 2) uniform MaterialData {
   float ao;
   float roughness;
   float metallic;
   vec3 albedo;
} material;

layout(location = 0) out vec4 gAlbedo; // RGB color + A AO
layout(location = 1) out vec4 gNormal; // RG encoded normal + B roughness + A metallic

layout(binding = 0) uniform sampler2D albedoSampler;
layout(binding = 1) uniform sampler2D normalSampler;
layout(binding = 2) uniform sampler2D roughnessSampler;
layout(binding = 3) uniform sampler2D metallicSampler;
layout(binding = 4) uniform sampler2D aoSampler;

mat3 computeTBN(vec3 N, vec2 uv, vec3 pos) {
   vec3 dp1 = dFdx(pos);
   vec3 dp2 = dFdy(pos);
   vec2 duv1 = dFdx(uv);
   vec2 duv2 = dFdy(uv);
   vec3 T = normalize(duv2.y * dp1 - duv1.y * dp2);
   vec3 B = normalize(-duv2.x * dp1 + duv1.x * dp2);
   return mat3(T, B, N);
}

vec2 encodeOctNormal(vec3 n) {
   n /= (abs(n.x) + abs(n.y) + abs(n.z));
   vec2 enc = n.xy;
   if (n.z < 0.0) {
      enc = (1.0 - abs(enc.yx)) * sign(enc.xy);
   }
   return enc * 0.5 + 0.5;
}

void main() {
   // Compute TBN from original geometry
   mat3 TBN = computeTBN(normalize(fragNormal), fragUV, fragPos);
   // Compute view direction in world space and convert to tangent space
   vec3 viewDir = normalize(camera.viewPos - fragPos);
   vec3 viewDirTS = normalize(TBN * viewDir);
   gAlbedo = vec4(1.0);
   gNormal = vec4(encodeOctNormal(fragNormal), 1.0, 0.0);
}
