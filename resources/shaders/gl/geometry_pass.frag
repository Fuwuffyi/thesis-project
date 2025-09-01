#version 460

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragUV;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
} camera;

// Depth R is used to calculate position
layout(location = 0) out vec4 gAlbedo; // RGB color + A AO
layout(location = 1) out vec4 gNormal; // RG encoded normal + B roughness + A metallic

layout(binding = 3) uniform sampler2D texAlbedo;
layout(binding = 4) uniform sampler2D texDisplacement;
layout(binding = 5) uniform sampler2D texNormal;
layout(binding = 6) uniform sampler2D texRoughness;
layout(binding = 7) uniform sampler2D texAO;

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
   // Parallax offset using displacement map
   float height = texture(texDisplacement, fragUV).r;
   float parallaxScale = 0.05;
   vec2 parallaxUV = fragUV + viewDirTS.xy * (height * parallaxScale);
   // Sample normal in tangent space
   vec3 normalTS = texture(texNormal, parallaxUV).rgb * 2.0 - 1.0;
   vec3 normalWS = normalize(TBN * normalTS);
   // Write g-buffer
   gAlbedo = vec4(texture(texAlbedo, parallaxUV).rgb, texture(texAO, parallaxUV).r);
   gNormal = vec4(encodeOctNormal(normalWS), texture(texRoughness, parallaxUV).r, 0.0);
}

