#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
} camera;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragUV;

uniform mat4 model;

void main() {
   vec4 worldPos = model * vec4(inPosition, 1.0);

   mat3 normalMatrix = transpose(inverse(mat3(model)));
   fragNormal = normalize(normalMatrix * inNormal);

   fragColor = inNormal;
   fragUV = inUV;

   gl_Position = camera.proj * camera.view * worldPos;
}
