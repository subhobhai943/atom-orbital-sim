// =============================================================================
// Standalone 2D Wavefunction Slice & Nodal Surface Visualizer
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "../../include/atom_state.h"
#include "../../include/camera.h"
#include "../../include/renderer_wave_2d.h"

static orbital::ArcballCamera g_camera;
static orbital::SimulationSettings g_settings;
static orbital::Wave2DRenderer g_renderer;
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
    else if (key == GLFW_KEY_UP) {
        g_settings.n = std::min(g_settings.n + 1, 5);
        g_settings.clampQuantumNumbers();
    } else if (key == GLFW_KEY_DOWN) {
        g_settings.n = std::max(g_settings.n - 1, 1);
        g_settings.clampQuantumNumbers();
    } else if (key == GLFW_KEY_RIGHT) {
        g_settings.l = std::min(g_settings.l + 1, g_settings.n - 1);
        g_settings.clampQuantumNumbers();
    } else if (key == GLFW_KEY_LEFT) {
        g_settings.l = std::max(g_settings.l - 1, 0);
        g_settings.clampQuantumNumbers();
    } else if (key == GLFW_KEY_N) {
        g_settings.show_nodal_lines = !g_settings.show_nodal_lines;
    } else if (key == GLFW_KEY_C) {
        g_settings.show_contours = !g_settings.show_contours;
    }
}

int main() {
    std::cout << "Starting 2D Wavefunction Slice & Nodal Surface Visualizer...\n"
              << "Controls:\n"
              << "  [UP/DOWN]: Change n (Principal)\n"
              << "  [LEFT/RIGHT]: Change l (Azimuthal)\n"
              << "  [N]: Toggle Nodal Lines (ψ = 0)\n"
              << "  [C]: Toggle Iso-Probability Contours\n"
              << "  [Mouse]: Orbit / Pan / Zoom\n";

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "2D Wavefunction Slice & Nodal Visualizer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    glewInit();

    g_renderer.init();
    g_settings.setOrbital(2, 1, 0);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

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
