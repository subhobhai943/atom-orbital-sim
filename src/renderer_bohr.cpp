#include "../include/renderer_bohr.h"
#include <iostream>
#include <cmath>

namespace orbital {

BohrRenderer::BohrRenderer() = default;

BohrRenderer::~BohrRenderer() {
    if (m_orbitVao) glDeleteVertexArrays(1, &m_orbitVao);
    if (m_orbitVbo) glDeleteBuffers(1, &m_orbitVbo);
    if (m_sphereVao) glDeleteVertexArrays(1, &m_sphereVao);
    if (m_sphereVbo) glDeleteBuffers(1, &m_sphereVbo);
    if (m_sphereEbo) glDeleteBuffers(1, &m_sphereEbo);
}

bool BohrRenderer::init() {
    m_shader = std::make_unique<Shader>();
    if (!m_shader->loadFromFiles("shaders/bohr.vert", "shaders/bohr.frag")) {
        const std::string vertSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)";
        const std::string fragSrc = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";
        m_shader->loadFromStrings(vertSrc, fragSrc);
    }

    glGenVertexArrays(1, &m_orbitVao);
    glGenBuffers(1, &m_orbitVbo);

    initSphereMesh();
    return true;
}

void BohrRenderer::initSphereMesh() {
    m_sphereShader = std::make_unique<Shader>();
    if (!m_sphereShader->loadFromFiles("shaders/nucleus.vert", "shaders/nucleus.frag")) {
        // Fallback
    }

    const int stacks = 16;
    const int slices = 24;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = float(M_PI) * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * float(M_PI) * float(j) / float(slices);

            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    m_sphereIndexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m_sphereVao);
    glGenBuffers(1, &m_sphereVbo);
    glGenBuffers(1, &m_sphereEbo);

    glBindVertexArray(m_sphereVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_sphereVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void BohrRenderer::update(float deltaTime, SimulationSettings& settings) {
    m_animTime += deltaTime;
    // Electron orbital speed decreases with higher n: v = v0 / n
    float speed = (2.5f / static_cast<float>(settings.bohr_n));
    settings.bohr_orbit_angle += speed * deltaTime;
    if (settings.bohr_orbit_angle > 2.0f * float(M_PI)) {
        settings.bohr_orbit_angle -= 2.0f * float(M_PI);
    }
}

void BohrRenderer::renderSphere(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float radius, const glm::vec3& color) {
    if (!m_sphereShader || !m_sphereVao) return;

    m_sphereShader->use();
    glm::mat4 model = glm::translate(glm::mat4(1.0f), pos);
    model = glm::scale(model, glm::vec3(radius));

    m_sphereShader->setMat4("uModel", model);
    m_sphereShader->setMat4("uView", view);
    m_sphereShader->setMat4("uProjection", proj);
    m_sphereShader->setVec3("uColor", color);
    m_sphereShader->setVec3("uCameraPos", glm::vec3(0, 0, 50));
    m_sphereShader->setFloat("uPulse", 0.5f);

    glBindVertexArray(m_sphereVao);
    glDrawElements(GL_TRIANGLES, m_sphereIndexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void BohrRenderer::render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float aspect = float(viewportWidth) / float(viewportHeight);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);

    // Build orbit rings and de Broglie standing matter waves
    std::vector<float> lineData; // pos(3), col(4)
    const int segments = 240;
    const float baseR = 3.5f;

    for (int n = 1; n <= 4; ++n) {
        float r = baseR * (n * n); // r_n = n^2 * a0
        bool isActive = (n == settings.bohr_n);
        glm::vec4 orbitCol = isActive ? glm::vec4(0.2f, 0.8f, 1.0f, 0.8f) : glm::vec4(0.3f, 0.3f, 0.4f, 0.35f);

        // Circular orbit line
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * float(M_PI) * float(i) / float(segments);
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            float y = 0.0f;

            lineData.push_back(x);
            lineData.push_back(y);
            lineData.push_back(z);
            lineData.push_back(orbitCol.r);
            lineData.push_back(orbitCol.g);
            lineData.push_back(orbitCol.b);
            lineData.push_back(orbitCol.a);
        }

        // de Broglie standing wave on the active orbit: exactly n sine waves around circumference!
        if (isActive && settings.bohr_show_wave) {
            glm::vec4 waveCol(1.0f, 0.85f, 0.2f, 0.95f);
            float waveAmp = 0.18f * r;

            for (int i = 0; i <= segments; ++i) {
                float theta = 2.0f * float(M_PI) * float(i) / float(segments);
                // de Broglie wave condition: n wavelengths in 2*pi
                float r_wave = r + waveAmp * std::sin(float(n) * theta - m_animTime * 6.0f);
                float x = r_wave * std::cos(theta);
                float z = r_wave * std::sin(theta);
                float y = waveAmp * 0.5f * std::cos(float(n) * theta - m_animTime * 6.0f);

                lineData.push_back(x);
                lineData.push_back(y);
                lineData.push_back(z);
                lineData.push_back(waveCol.r);
                lineData.push_back(waveCol.g);
                lineData.push_back(waveCol.b);
                lineData.push_back(waveCol.a);
            }
        }
    }

    glBindVertexArray(m_orbitVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_orbitVbo);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    m_shader->use();
    m_shader->setMat4("uModel", glm::mat4(1.0f));
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);

    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineData.size() / 7));
    glBindVertexArray(0);

    // Render Nucleus (Proton) at center
    renderSphere(view, proj, glm::vec3(0.0f), 1.2f, glm::vec3(0.95f, 0.15f, 0.15f));

    // Render Orbiting Electron on active shell
    float activeR = baseR * (settings.bohr_n * settings.bohr_n);
    glm::vec3 electronPos(
        activeR * std::cos(settings.bohr_orbit_angle),
        0.0f,
        activeR * std::sin(settings.bohr_orbit_angle)
    );
    renderSphere(view, proj, electronPos, 0.65f, glm::vec3(0.1f, 0.95f, 1.0f));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace orbital
