#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aInstancePos;
layout(location = 2) in float aInstanceRadius;
layout(location = 3) in vec3 aInstanceColor;

uniform mat4 uProjection;
uniform mat4 uView;

out vec2 vLocalPos;
out vec3 vColor;

void main() {
    vLocalPos = aPos;
    vColor = aInstanceColor;
    vec2 worldPos = aPos * aInstanceRadius + aInstancePos;
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}