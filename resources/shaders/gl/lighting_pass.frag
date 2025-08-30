#version 460

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
} camera;

layout(binding = 1) uniform sampler2D gAlbedoSampler;
layout(binding = 2) uniform sampler2D gNormalSampler;
layout(binding = 3) uniform sampler2D gDepthSampler;

vec3 getWorldPos(vec2 uv, float depth) {
   // Convert UV [0,1] -> NDC [-1,1]
   vec2 ndc = uv * 2.0 - 1.0;
   float z = depth * 2.0 - 1.0; // depth in NDC
   vec4 clipPos = vec4(ndc, z, 1.0);
   // Compute inverse projection & inverse view
   mat4 invProj = inverse(camera.proj);
   mat4 invView = inverse(camera.view);
   // Back to view space
   vec4 viewPos = invProj * clipPos;
   viewPos /= viewPos.w;
   // Back to world space
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

void main() {
   vec3 albedo = texture(gAlbedoSampler, fragUV).rgb;
   vec3 normal = decodeOctNormal(texture(gNormalSampler, fragUV).xy);
   float depth = texture(gDepthSampler, fragUV).r;
   vec3 worldPos = getWorldPos(fragUV, depth);

   fragColor = texture(gAlbedoSampler, fragUV) * vec4(normal, 1.0);
}
