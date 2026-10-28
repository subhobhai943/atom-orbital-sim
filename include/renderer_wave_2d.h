#ifndef RENDERER_WAVE_2D_H
#define RENDERER_WAVE_2D_H

#include "atom_state.h"
#include "camera.h"
#include "shader.h"
#include <GL/glew.h>
#include <memory>

namespace orbital {

class Wave2DRenderer {
public:
    Wave2DRenderer();
    ~Wave2DRenderer();

    bool init();
    void render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight);

private:
    GLuint m_planeVao = 0;
    GLuint m_planeVbo = 0;
    std::unique_ptr<Shader> m_shader;
};

} // namespace orbital

#endif // RENDERER_WAVE_2D_H
