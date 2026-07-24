#version 330 core
in vec2 vLocalPos;
out vec4 fragColor;

void main() {
    float dist = length(vLocalPos);
    float radius = 1.0;

    float angle = atan(vLocalPos.y, vLocalPos.x);
    float spikes = sin(angle * 10.0) * 0.15;
    float shape = radius + spikes;

    float alpha = 1.0 - smoothstep(shape - 0.02, shape, dist);
    if (alpha < 0.01) discard;

    vec3 coreColor = vec3(0.1, 0.5, 0.1);
    vec3 spikeColor = vec3(0.0, 0.35, 0.0);
    vec3 finalColor = mix(spikeColor, coreColor, smoothstep(shape - 0.1, shape - 0.05, dist));

    fragColor = vec4(finalColor, alpha);
}