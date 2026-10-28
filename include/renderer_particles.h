#ifndef RENDERER_PARTICLES_H
#define RENDERER_PARTICLES_H

#include "atom_state.h"
#include "camera.h"
#include "shader.h"
#include <GL/glew.h>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace orbital {

struct ParticleVertex {
    glm::vec3 pos;
    glm::vec4 color;
};

class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    bool init();
    void generateParticles(SimulationSettings& settings);
    void update(float deltaTime, SimulationSettings& settings);
    void render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight);

    int getActualCount() const { return static_cast<int>(m_particles.size()); }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::unique_ptr<Shader> m_shader;

    struct ParticleSim {
        glm::vec3 base_pos;
        glm::vec3 current_pos;
        glm::vec4 color;
        float radius;
        float theta;
        float phi;
    };

    std::vector<ParticleSim> m_particles;
    std::vector<ParticleVertex> m_gpuBuffer;

    // Nucleus rendering
    GLuint m_nucleusVao = 0;
    GLuint m_nucleusVbo = 0;
    GLuint m_nucleusEbo = 0;
    int m_nucleusIndexCount = 0;
    std::unique_ptr<Shader> m_nucleusShader;
    void initNucleusMesh();
    void renderNucleus(const ArcballCamera& camera, const SimulationSettings& settings, float time);
};

} // namespace orbital

#endif // RENDERER_PARTICLES_H
