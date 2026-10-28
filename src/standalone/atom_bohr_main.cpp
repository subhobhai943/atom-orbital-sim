// =============================================================================
// Standalone 3D Bohr Model vs Quantum Mechanics Visualizer
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "../../include/atom_state.h"
#include "../../include/camera.h"
#include "../../include/renderer_bohr.h"
#include "../../include/renderer_particles.h"

static orbital::ArcballCamera g_camera;
static orbital::SimulationSettings g_settings;
static orbital::BohrRenderer g_bohrRenderer;
static orbital::ParticleRenderer g_particleRenderer;
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
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_4) {
        g_settings.bohr_n = key - GLFW_KEY_0;
        g_settings.setOrbital(g_settings.bohr_n, 0, 0);
        g_particleRenderer.generateParticles(g_settings);
    } else if (key == GLFW_KEY_W) {
        g_settings.bohr_show_wave = !g_settings.bohr_show_wave;
    } else if (key == GLFW_KEY_Q) {
        g_settings.bohr_show_cloud_overlay = !g_settings.bohr_show_cloud_overlay;
    }
}

int main() {
    std::cout << "Starting 3D Bohr vs Quantum Mechanics Visualizer...\n"
              << "Controls:\n"
              << "  [1-4]: Change Bohr Orbit Shell (n = 1..4)\n"
              << "  [W]: Toggle de Broglie Standing Matter Wave\n"
              << "  [Q]: Toggle Quantum Cloud Overlay\n"
              << "  [Space]: Toggle Auto-Rotate\n"
              << "  [Mouse]: Orbit / Pan / Zoom\n";

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "3D Bohr Model vs Quantum Mechanics", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    glewInit();

    g_bohrRenderer.init();
    g_particleRenderer.init();
    g_settings.bohr_n = 2;
    g_settings.setOrbital(2, 0, 0);
    g_particleRenderer.generateParticles(g_settings);

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        glfwPollEvents();
        g_camera.update(dt);
        g_bohrRenderer.update(dt, g_settings);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_bohrRenderer.render(g_camera, g_settings, w, h);
        if (g_settings.bohr_show_cloud_overlay) {
            g_particleRenderer.render(g_camera, g_settings, w, h);
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
