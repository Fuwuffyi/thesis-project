#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, set = 0, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
} camera;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

layout(push_constant) uniform ObjectData {
   mat4 model;
} object;

void main() {
   // Get object world position
   vec4 worldPos = object.model * vec4(inPosition, 1.0);
   // Use model matrix to transform normals to world space
   mat3 normalMatrix = transpose(inverse(mat3(object.model)));
   fragNormal = normalize(normalMatrix * inNormal);
   // Interpolate other values to fragment shader
   fragPos = worldPos.xyz;
   fragUV = inUV;
   // Move object according to camera as well
   gl_Position = camera.proj * camera.view * worldPos;
}
