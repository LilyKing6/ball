#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aInstancePos;
layout(location = 2) in vec4 aInstanceColor;
layout(location = 3) in float aInstanceRadius;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 vColor;
out vec2 vLocalPos;

void main() {
    vColor = aInstanceColor;
    vLocalPos = aPos;
    vec2 worldPos = aPos * aInstanceRadius + aInstancePos;
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}
