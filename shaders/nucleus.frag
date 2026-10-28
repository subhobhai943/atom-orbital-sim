#version 330 core

in vec3 vNormal;
in vec3 vFragPos;

uniform vec3 uColor;
uniform vec3 uCameraPos;
uniform float uPulse;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.7));
    vec3 V = normalize(uCameraPos - vFragPos);

    // Blinn-phong shading
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    // Fresnel rim glow
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    vec3 ambient = 0.25 * uColor;
    vec3 diffuse = diff * uColor;
    vec3 specular = spec * vec3(1.0);
    vec3 rim = fresnel * (uColor * 0.8 + vec3(0.2)) * (0.8 + 0.2 * uPulse);

    vec3 result = ambient + diffuse + specular + rim;
    FragColor = vec4(result, 1.0);
}
