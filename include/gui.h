#ifndef GUI_H
#define GUI_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "atom_state.h"
#include "camera.h"

namespace orbital {

class ParticleRenderer;
class RaytracerRenderer;
class ExcitationRenderer;
class Wave2DRenderer;
class BohrRenderer;

class Gui {
public:
    Gui() = default;
    ~Gui() = default;

    bool init(GLFWwindow* window);
    void newFrame();
    void render(SimulationSettings& settings,
                ArcballCamera& camera,
                ParticleRenderer& particleRenderer,
                ExcitationRenderer& excitationRenderer,
                float fps,
                GLFWwindow* window);
    void draw();
    void shutdown();

private:
    void renderMiniHud(SimulationSettings& settings, ParticleRenderer& particleRenderer, ExcitationRenderer& excitationRenderer, GLFWwindow* window, float fps);
    void renderHiddenHudOverlay(SimulationSettings& settings, float fps);
    void renderModeSelection(SimulationSettings& settings, ParticleRenderer& particleRenderer, ExcitationRenderer& excitationRenderer);
    void renderOrbitalControls(SimulationSettings& settings, ParticleRenderer& particleRenderer);
    void renderPresets(SimulationSettings& settings, ParticleRenderer& particleRenderer);
    void renderVisualSettings(SimulationSettings& settings, ParticleRenderer& particleRenderer);
    void renderExcitationControls(SimulationSettings& settings, ExcitationRenderer& excitationRenderer);
    void renderPhysicsTelemetry(const SimulationSettings& settings);
    void renderCameraControls(ArcballCamera& camera);
};

} // namespace orbital

#endif // GUI_H
