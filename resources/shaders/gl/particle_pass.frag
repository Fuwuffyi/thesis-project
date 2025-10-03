#version 460 core

in vec4 fragColor;
in vec2 fragTexCoord;

out vec4 FragColor;

void main() {
    vec2 coord = fragTexCoord * 2.0 - 1.0;
    float dist = length(coord);
    if (dist > 1.0)
        discard;
    float alpha = 1.0 - smoothstep(0.7, 1.0, dist);
    FragColor = vec4(fragColor.rgb, fragColor.a * alpha);
}
