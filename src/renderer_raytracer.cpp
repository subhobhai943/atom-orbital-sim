#include "../include/renderer_raytracer.h"
#include <iostream>

namespace orbital {

RaytracerRenderer::RaytracerRenderer() = default;

RaytracerRenderer::~RaytracerRenderer() {
    if (m_quadVao) glDeleteVertexArrays(1, &m_quadVao);
    if (m_quadVbo) glDeleteBuffers(1, &m_quadVbo);
}

bool RaytracerRenderer::init() {
    m_shader = std::make_unique<Shader>();
    if (!m_shader->loadFromFiles("shaders/raymarch.vert", "shaders/raymarch.frag")) {
        std::cerr << "Failed to load raymarching shaders from files, check shaders/ dir.\n";
        return false;
    }

    // Fullscreen NDC quad
    const float quadVertices[] = {
        // pos (x, y)
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f, -1.0f,

        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_quadVao);
    glGenBuffers(1, &m_quadVbo);

    glBindVertexArray(m_quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);
    return true;
}

void RaytracerRenderer::render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight) {
    if (!m_shader || !m_quadVao) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    m_shader->use();

    float aspect = float(viewportWidth) / float(viewportHeight);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 invViewProj = glm::inverse(proj * view);

    m_shader->setVec3("uCameraPos", camera.getPosition());
    m_shader->setMat4("uInvViewProj", invViewProj);
    m_shader->setInt("uN", settings.n);
    m_shader->setInt("uL", settings.l);
    m_shader->setInt("uM", settings.m);
    m_shader->setBool("uRealOrbitals", settings.use_real_orbitals);
    m_shader->setInt("uPalette", static_cast<int>(settings.palette));
    m_shader->setFloat("uDensity", settings.volume_density);

    // Dynamic bounding radius proportional to n^2
    float boundRadius = std::max(15.0f, 3.5f * float(settings.n * settings.n) + 8.0f);
    m_shader->setFloat("uBoundRadius", boundRadius);
    m_shader->setInt("uMaxSteps", settings.ray_steps);

    m_shader->setBool("uEnableClip", settings.enable_slice);
    glm::vec4 clipPlane(0.0f, 1.0f, 0.0f, -settings.slice_height);
    if (settings.slice_axis == 0) clipPlane = glm::vec4(1.0f, 0.0f, 0.0f, -settings.slice_height);
    else if (settings.slice_axis == 2) clipPlane = glm::vec4(0.0f, 0.0f, 1.0f, -settings.slice_height);
    m_shader->setVec4("uClipPlane", clipPlane);

    glBindVertexArray(m_quadVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

} // namespace orbital
