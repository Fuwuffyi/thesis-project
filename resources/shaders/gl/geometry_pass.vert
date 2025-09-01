#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
   vec3 viewDir;
} camera;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

uniform mat4 model;

void main() {
   // Get object world position
   vec4 worldPos = model * vec4(inPosition, 1.0);
   // Use model matrix to transform normals to world space
   mat3 normalMatrix = transpose(inverse(mat3(model)));
   fragNormal = normalize(normalMatrix * inNormal);
   // Interpolate other values to fragment shader
   fragColor = inNormal;
   fragUV = inUV;
   // Move object according to camera as well
   gl_Position = camera.proj * camera.view * worldPos;
}
