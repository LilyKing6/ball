#version 330 core
in vec4 vColor;
in vec2 vLocalPos;
in float vAlpha;
in float vRadius;
in float vPoison;

out vec4 fragColor;

void main() {
    vec2 uv = vLocalPos;
    float dist = length(uv);
    float radius = 1.0;

    // 屏幕空间抗锯齿：使用 fwidth 计算像素宽度
    float pixelWidth = fwidth(dist);
    float aaWidth = max(pixelWidth, 0.005);

    float edge = 1.0 - smoothstep(radius - aaWidth, radius, dist);
    if (edge < 0.001) discard;

    vec3 lightCol = mix(vColor.rgb, vec3(1.0), 0.3);
    vec3 darkCol = vColor.rgb * 0.6;
    vec3 gradient = mix(lightCol, darkCol, dist / radius);

    float outlineWidth = max(0.03, pixelWidth * 2.0);
    float outline = smoothstep(radius - outlineWidth, radius, dist)
                  - smoothstep(radius - outlineWidth * 0.5, radius - outlineWidth, dist);
    vec3 outlineCol = vColor.rgb * 0.4;
    vec3 finalCol = mix(gradient, outlineCol, outline);

    // 中毒：紫色 mix + 半透明
    if (vPoison > 0.0) {
        vec3 poisonColor = vec3(0.6, 0.0, 0.8);
        finalCol = mix(finalCol, poisonColor, vPoison * 0.5);
    }

    float finalAlpha = edge * vAlpha;
    if (vPoison > 0.0) {
        finalAlpha *= mix(1.0, 0.75, vPoison);
    }

    fragColor = vec4(finalCol, finalAlpha);
}
