#version 460

#define PI 3.14159265358979

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 fragColor;

struct LightData {
   uint lightType; // 0 = Dir, 1 = Point, 2 = Spot
   vec3 position;
   vec3 direction;
   vec3 color;
   float intensity;
   float constant;
   float linear;
   float quadratic;
   float innerCone;
   float outerCone;
};

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
} camera;

#define MAX_LIGHTS 256
layout(std140, binding = 1) uniform LightsData {
   uint lightCount;
   LightData lights[MAX_LIGHTS];
} lights;

layout(binding = 3) uniform sampler2D gAlbedo;   // RGB color + A AO
layout(binding = 4) uniform sampler2D gNormal;   // RG encoded normal + B roughness + A metallic
layout(binding = 5) uniform sampler2D gDepth;    // R depth value

// === G-Buffer Utility Functions ===

vec3 getWorldPos(vec2 uv, float depth) {
   vec2 ndc = uv * 2.0 - 1.0;
   float z = depth * 2.0 - 1.0;
   vec4 clipPos = vec4(ndc, z, 1.0);
   mat4 invProj = inverse(camera.proj);
   mat4 invView = inverse(camera.view);
   vec4 viewPos = invProj * clipPos;
   viewPos /= viewPos.w;
   vec4 worldPos = invView * viewPos;
   return worldPos.xyz;
}

vec3 decodeOctNormal(vec2 enc) {
   enc = enc * 2.0 - 1.0;
   vec3 n = vec3(enc.xy, 1.0 - abs(enc.x) - abs(enc.y));
   if (n.z < 0.0) {
      n.xy = (1.0 - abs(n.yx)) * sign(n.xy);
   }
   return normalize(n);
}

// === PBR Functions ===

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
   return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
   float a = roughness * roughness;
   float a2 = a * a;
   float NdotH = max(dot(N, H), 0.0);
   float NdotH2 = NdotH * NdotH;
   float denom = (NdotH2 * (a2 - 1.0) + 1.0);
   denom = PI * denom * denom;
   return a2 / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
   float r = roughness + 1.0;
   float k = (r * r) / 8.0;
   return NdotV / (NdotV * (1.0 - k) + k);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
   float ggx1 = geometrySchlickGGX(max(dot(N, V), 0.0), roughness);
   float ggx2 = geometrySchlickGGX(max(dot(N, L), 0.0), roughness);
   return ggx1 * ggx2;
}

// === Lighting calculation ===

void main() {
   vec4 gNormalTex = texture(gNormal, fragUV);
   vec3 albedo = texture(gAlbedo, fragUV).rgb;
   float roughness = gNormalTex.b;
   float metallic = gNormalTex.a;
   float ao = texture(gAlbedo, fragUV).a;
   float depth = texture(gDepth, fragUV).r;
   vec3 worldPos = getWorldPos(fragUV, depth);
   vec3 N = normalize(decodeOctNormal(gNormalTex.xy));
   vec3 V = normalize(camera.viewPos - worldPos);
   vec3 F0 = mix(vec3(0.04), albedo, metallic);
   vec3 finalColor = vec3(0.0);
   for (uint i = 0; i < lights.lightCount; ++i) {
      vec3 L;
      vec3 radiance = lights.lights[i].color * lights.lights[i].intensity;
      if (lights.lights[i].lightType == 0u) { // Directional
         L = normalize(-lights.lights[i].direction);
      } else {
         L = normalize(lights.lights[i].position - worldPos);
         float dist = length(lights.lights[i].position - worldPos);
         float attenuation = 1.0 / (lights.lights[i].constant +
               lights.lights[i].linear * dist +
               lights.lights[i].quadratic * dist * dist);
         radiance *= attenuation;
         // Spotlight cone
         if (lights.lights[i].lightType == 2u) {
            float theta = dot(L, normalize(-lights.lights[i].direction));
            float epsilon = lights.lights[i].innerCone - lights.lights[i].outerCone;
            float intensity = clamp((theta - lights.lights[i].outerCone) / epsilon, 0.0, 1.0);
            radiance *= intensity;
         }
      }
      // PBR shading
      vec3 H = normalize(V + L);
      float NDF = distributionGGX(N, H, roughness);
      float G = geometrySmith(N, V, L, roughness);
      vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
      vec3 numerator = NDF * G * F;
      float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
      vec3 specular = numerator / denominator;
      vec3 kS = F;
      vec3 kD = (1.0 - kS) * (1.0 - metallic);
      float NdotL = max(dot(N, L), 0.0);
      finalColor += (kD * albedo / PI + specular) * radiance * NdotL;
   }
   // AO
   vec3 ambient = vec3(0.03) * albedo * ao;
   finalColor += ambient;
   fragColor = vec4(finalColor, 1.0);
}
