#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aInstancePos;
layout(location = 2) in vec4 aInstanceColor;
layout(location = 3) in float aInstanceRadius;
layout(location = 4) in float aInstanceAlpha;
layout(location = 5) in float aPoison;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uTime;

out vec4 vColor;
out vec2 vLocalPos;
out float vAlpha;
out float vRadius;
out float vPoison;

void main() {
    vColor = aInstanceColor;
    vLocalPos = aPos;
    vAlpha = aInstanceAlpha;
    vRadius = aInstanceRadius;
    vPoison = aPoison;

    // 中毒抖动：振幅与 aPoison 成正比，5% radius 上限
    float t = uTime * 30.0;
    float jitter = aPoison * aInstanceRadius * 0.05;
    vec2 offset = vec2(sin(t + aInstancePos.x * 0.1), cos(t + aInstancePos.y * 0.1)) * jitter;

    vec2 worldPos = aPos * aInstanceRadius + aInstancePos + offset;
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}
