#ifndef RENDERER_BOHR_H
#define RENDERER_BOHR_H

#include "atom_state.h"
#include "camera.h"
#include "shader.h"
#include <GL/glew.h>
#include <vector>
#include <memory>

namespace orbital {

class BohrRenderer {
public:
    BohrRenderer();
    ~BohrRenderer();

    bool init();
    void update(float deltaTime, SimulationSettings& settings);
    void render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight);

private:
    GLuint m_orbitVao = 0;
    GLuint m_orbitVbo = 0;
    std::unique_ptr<Shader> m_shader;

    GLuint m_sphereVao = 0;
    GLuint m_sphereVbo = 0;
    GLuint m_sphereEbo = 0;
    int m_sphereIndexCount = 0;
    std::unique_ptr<Shader> m_sphereShader;

    float m_animTime = 0.0f;
    void initSphereMesh();
    void renderSphere(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float radius, const glm::vec3& color);
};

} // namespace orbital

#endif // RENDERER_BOHR_H
