#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 fragColor;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
} camera;

uniform mat4 model;

void main() {
   gl_Position = camera.proj * camera.view * model * vec4(inPosition, 1.0);
   fragColor = inNormal;
}
