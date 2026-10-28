#version 330 core

in vec4 vColor;
in float vDist;

uniform float uBrightness;

out vec4 FragColor;

void main() {
    // gl_PointCoord is [0, 1] across the point sprite quad
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distSq = dot(coord, coord) * 4.0; // 0 at center, 1 at circle edge

    if (distSq > 1.0) {
        discard;
    }

    // Soft Gaussian-like glow profile
    float falloff = exp(-3.2 * distSq) * (1.0 - distSq);
    float alpha = clamp(vColor.a * falloff * uBrightness, 0.0, 1.0);

    vec3 rgb = vColor.rgb * (1.0 + 0.3 * exp(-6.0 * distSq)); // Subtle hot core
    FragColor = vec4(rgb, alpha);
}
