#include "../include/renderer_wave_2d.h"
#include <iostream>

namespace orbital {

Wave2DRenderer::Wave2DRenderer() = default;

Wave2DRenderer::~Wave2DRenderer() {
    if (m_planeVao) glDeleteVertexArrays(1, &m_planeVao);
    if (m_planeVbo) glDeleteBuffers(1, &m_planeVbo);
}

bool Wave2DRenderer::init() {
    m_shader = std::make_unique<Shader>();
    if (!m_shader->loadFromFiles("shaders/wave_slice.vert", "shaders/wave_slice.frag")) {
        std::cerr << "Failed to load wave slice shader from files, check shaders/ dir.\n";
        return false;
    }

    // High-resolution quad representing the 2D cross-section
    const float extent = 35.0f;
    const float planeVertices[] = {
        // x, z (in plane coordinates)
        -extent, -extent,
        -extent,  extent,
         extent,  extent,

        -extent, -extent,
         extent,  extent,
         extent, -extent
    };

    glGenVertexArrays(1, &m_planeVao);
    glGenBuffers(1, &m_planeVbo);

    glBindVertexArray(m_planeVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_planeVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    return true;
}

void Wave2DRenderer::render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight) {
    if (!m_shader || !m_planeVao) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    m_shader->use();

    float aspect = float(viewportWidth) / float(viewportHeight);
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);

    m_shader->setMat4("uModel", model);
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);

    m_shader->setInt("uN", settings.n);
    m_shader->setInt("uL", settings.l);
    m_shader->setInt("uM", settings.m);
    m_shader->setBool("uRealOrbitals", settings.use_real_orbitals);
    m_shader->setBool("uShowContours", settings.show_contours);
    m_shader->setBool("uShowNodalLines", settings.show_nodal_lines);
    m_shader->setFloat("uBrightness", settings.brightness);

    glBindVertexArray(m_planeVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace orbital
