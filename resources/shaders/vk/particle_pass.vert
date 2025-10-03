#version 460

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 3) in mat4 instanceTransform;
layout(location = 7) in vec4 instanceColor;

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} camera;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // Extract camera rotation
    mat3 camRot = transpose(mat3(camera.view));
    vec3 scale;
    scale.x = length(vec3(instanceTransform[0]));
    scale.y = length(vec3(instanceTransform[1]));
    scale.z = length(vec3(instanceTransform[2]));
    // Get instance position
    vec3 instancePos = vec3(instanceTransform[3]);
    // Billboard
    vec3 worldPos = instancePos + camRot * (aPos * scale);
    gl_Position = camera.proj * camera.view * vec4(worldPos, 1.0);
    fragColor = instanceColor;
    fragTexCoord = aTexCoord;
}
