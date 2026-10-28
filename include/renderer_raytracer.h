#ifndef RENDERER_RAYTRACER_H
#define RENDERER_RAYTRACER_H

#include "atom_state.h"
#include "camera.h"
#include "shader.h"
#include <GL/glew.h>
#include <memory>

namespace orbital {

class RaytracerRenderer {
public:
    RaytracerRenderer();
    ~RaytracerRenderer();

    bool init();
    void render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight);

private:
    GLuint m_quadVao = 0;
    GLuint m_quadVbo = 0;
    std::unique_ptr<Shader> m_shader;
};

} // namespace orbital

#endif // RENDERER_RAYTRACER_H
