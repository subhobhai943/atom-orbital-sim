// =============================================================================
// Standalone Quantum Excitation & Transition Simulator
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "../../include/atom_state.h"
#include "../../include/camera.h"
#include "../../include/renderer_excitation.h"

static orbital::ArcballCamera g_camera;
static orbital::SimulationSettings g_settings;
static orbital::ExcitationRenderer g_renderer;
static int g_width = 1024, g_height = 768;

static void mouseButtonCallback(GLFWwindow*, int button, int action, int) {
    double x, y;
    glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
    g_camera.onMouseButton(button, action, x, y);
}

static void cursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    g_camera.onMouseMove(xpos, ypos);
}

static void scrollCallback(GLFWwindow*, double, double yoffset) {
    g_camera.onScroll(yoffset);
}

static void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;

    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
    else if (key == GLFW_KEY_R) g_camera.reset(45.0f);
    else if (key == GLFW_KEY_SPACE) g_camera.autoRotate = !g_camera.autoRotate;
    else if (key == GLFW_KEY_1) {
        // Excite 1s -> 2p
        g_settings.setOrbital(1, 0, 0);
        g_renderer.triggerTransition(g_settings, 2, 1, 0);
    } else if (key == GLFW_KEY_2) {
        // Emit 3p -> 2s (Balmer H-alpha Red 656nm)
        g_settings.setOrbital(3, 1, 0);
        g_renderer.triggerTransition(g_settings, 2, 0, 0);
    } else if (key == GLFW_KEY_3) {
        // Emit 4p -> 2s (Balmer H-beta Cyan 486nm)
        g_settings.setOrbital(4, 1, 0);
        g_renderer.triggerTransition(g_settings, 2, 0, 0);
    }
}

int main() {
    std::cout << "Starting Quantum Excitation & Transition Simulator...\n"
              << "Controls:\n"
              << "  [1]: Excite (1s -> 2p) Lyman-alpha\n"
              << "  [2]: Emit Balmer H-alpha (3p -> 2s, 656nm Red)\n"
              << "  [3]: Emit Balmer H-beta (4p -> 2s, 486nm Cyan)\n"
              << "  [Space]: Toggle Auto-Rotate\n"
              << "  [Mouse]: Orbit / Pan / Zoom\n";

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "Quantum Jump & Photon Emission Simulator", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    glewInit();

    g_renderer.init();
    g_settings.setOrbital(1, 0, 0);
    g_renderer.triggerTransition(g_settings, 2, 1, 0);

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        glfwPollEvents();
        g_camera.update(dt);
        g_renderer.update(dt, g_settings);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_renderer.render(g_camera, g_settings, w, h);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
