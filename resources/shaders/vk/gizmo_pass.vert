#version 460

layout(location = 0) in vec3 inPosition;

layout(std140, binding = 0) uniform CameraData {
   mat4 view;
   mat4 proj;
   vec3 viewPos;
} camera;

layout(push_constant) uniform ObjectData {
   mat4 model;
} object;

void main() {
   vec4 worldPos = object.model * vec4(inPosition, 1.0);
   gl_Position = camera.proj * camera.view * worldPos;
}
