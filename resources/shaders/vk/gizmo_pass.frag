#version 460

layout(push_constant) uniform GizmoData {
   vec3 color;
} object;

layout(location = 0) out vec4 fragColor;

void main() {
   fragColor = vec4(object.color, 1.0);
}

