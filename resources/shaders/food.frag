#version 330 core
in vec4 vColor;
in vec2 vLocalPos;

out vec4 fragColor;

void main() {
    float dist = length(vLocalPos);
    float alpha = 1.0 - smoothstep(0.85, 1.0, dist);
    vec3 col = mix(vColor.rgb, vec3(1.0), 0.2);
    fragColor = vec4(col, alpha * 0.95);
    if (fragColor.a < 0.01) discard;
}
