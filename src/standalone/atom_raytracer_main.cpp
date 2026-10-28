// =============================================================================
// Standalone GPU Volumetric Wavefunction Raymarcher
// =============================================================================

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "../../include/atom_state.h"
#include "../../include/camera.h"
#include "../../include/renderer_raytracer.h"

static orbital::ArcballCamera g_camera;
static orbital::SimulationSettings g_settings;
static orbital::RaytracerRenderer g_renderer;
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
    else if (key == GLFW_KEY_UP) {
        g_settings.n = std::min(g_settings.n + 1, 4);
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
    } else if (key == GLFW_KEY_M) {
        g_settings.m++;
        if (g_settings.m > g_settings.l) g_settings.m = -g_settings.l;
    } else if (key == GLFW_KEY_C) {
        int pal = (static_cast<int>(g_settings.palette) + 1) % 4;
        g_settings.palette = static_cast<orbital::ColorPalette>(pal);
    }
}

int main() {
    std::cout << "Starting Standalone GPU Wavefunction Raymarcher...\n"
              << "Controls:\n"
              << "  [UP/DOWN]: Change n (Principal)\n"
              << "  [LEFT/RIGHT]: Change l (Azimuthal)\n"
              << "  [M]: Cycle m (Magnetic)\n"
              << "  [C]: Cycle Color Palette\n"
              << "  [Space]: Toggle Auto-Rotate\n"
              << "  [Mouse]: Orbit / Pan / Zoom\n";

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(g_width, g_height, "3D Volumetric Wavefunction Raymarcher", nullptr, nullptr);
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

    auto lastTime = std::chrono::high_resolution_clock::now();
    while (!glfwWindowShouldClose(window)) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        glfwPollEvents();
        g_camera.update(dt);

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        glClearColor(0.01f, 0.01f, 0.03f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g_renderer.render(g_camera, g_settings, w, h);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
