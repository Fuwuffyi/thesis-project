#version 460

uniform vec3 gizmoColor;

out vec4 fragColor;

void main() {
   fragColor = vec4(gizmoColor, 1.0);
}

