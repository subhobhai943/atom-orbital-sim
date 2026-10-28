#ifndef RENDERER_EXCITATION_H
#define RENDERER_EXCITATION_H

#include "atom_state.h"
#include "camera.h"
#include "shader.h"
#include <GL/glew.h>
#include <vector>
#include <memory>

namespace orbital {

class ExcitationRenderer {
public:
    ExcitationRenderer();
    ~ExcitationRenderer();

    bool init();
    void ensureInitialized(SimulationSettings& settings);
    void triggerTransition(SimulationSettings& settings, int target_n, int target_l, int target_m);
    void update(float deltaTime, SimulationSettings& settings);
    void render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight);

    std::string getSpectralSeriesName(int n_lower) const;

private:
    GLuint m_particleVao = 0;
    GLuint m_particleVbo = 0;
    std::unique_ptr<Shader> m_particleShader;

    GLuint m_photonVao = 0;
    GLuint m_photonVbo = 0;
    std::unique_ptr<Shader> m_photonShader;

    struct ExcitationParticle {
        glm::vec3 pos1;
        glm::vec3 pos2;
        glm::vec3 current_pos;
        glm::vec4 color1;
        glm::vec4 color2;
        glm::vec4 current_color;
        float phase_offset;
    };

    std::vector<ExcitationParticle> m_particles;
    std::vector<float> m_particleBuffer; // pos(3), color(4)

    float m_animTime = 0.0f;
    float m_photonTravelDistance = 50.0f;
    float m_photonX = -50.0f;

    void rebuildSuperpositionParticles(SimulationSettings& settings);
    void renderPhotonWavepacket(const ArcballCamera& camera, const SimulationSettings& settings);
};

} // namespace orbital

#endif // RENDERER_EXCITATION_H
