#version 330 core
in vec2 vLocalPos;
in vec3 vColor;
out vec4 fragColor;

void main() {
    float dist = length(vLocalPos);
    float radius = 1.0;

    float angle = atan(vLocalPos.y, vLocalPos.x);
    float spikes = sin(angle * 10.0) * 0.15;
    float shape = radius + spikes;

    float alpha = 1.0 - smoothstep(shape - 0.02, shape, dist);
    if (alpha < 0.01) discard;

    // 用 vColor 做明暗差分：核心亮、刺尖暗，保持体积感
    vec3 coreColor = vColor * 1.15;
    vec3 spikeColor = vColor * 0.55;
    vec3 finalColor = mix(spikeColor, coreColor, smoothstep(shape - 0.1, shape - 0.05, dist));

    fragColor = vec4(finalColor, alpha);
}