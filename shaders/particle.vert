#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uBaseSize;
uniform bool uEnableClip;
uniform vec4 uClipPlane; // (nx, ny, nz, d)

out vec4 vColor;
out float vDist;

void main() {
    if (uEnableClip) {
        float dist = dot(aPos, uClipPlane.xyz) + uClipPlane.w;
        if (dist < 0.0) {
            // Discard point by moving outside NDC
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
            return;
        }
    }

    vec4 viewPos = uView * uModel * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;

    // Attenuate point size by camera depth
    float depth = max(-viewPos.z, 0.1);
    gl_PointSize = clamp(uBaseSize * (600.0 / depth), 1.5, 64.0);

    vColor = aColor;
    vDist = depth;
}
