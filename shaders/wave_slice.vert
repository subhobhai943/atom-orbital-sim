#version 330 core

layout (location = 0) in vec2 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vLocalPos;

void main() {
    vLocalPos = aPos;
    gl_Position = uProjection * uView * uModel * vec4(aPos.x, 0.0, aPos.y, 1.0);
}
