#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

#include "../include/atom_state.h"
#include "../include/camera.h"
#include "../include/renderer_particles.h"
#include "../include/renderer_raytracer.h"
#include "../include/renderer_excitation.h"
#include "../include/renderer_wave_2d.h"
#include "../include/renderer_bohr.h"
#include "../include/gui.h"
#include "../third_party/imgui/imgui.h"

// Global pointers for GLFW callbacks
static orbital::ArcballCamera* g_camera = nullptr;
static orbital::SimulationSettings* g_settings = nullptr;
static orbital::ParticleRenderer* g_particleRenderer = nullptr;
static orbital::ExcitationRenderer* g_excitationRenderer = nullptr;
static int g_windowWidth = 1280;
static int g_windowHeight = 800;
static bool g_isFullscreen = false;
static int g_savedWindowX = 100, g_savedWindowY = 100;
static int g_savedWindowW = 1280, g_savedWindowH = 800;

static void toggleFullscreen(GLFWwindow* window) {
    g_isFullscreen = !g_isFullscreen;
    if (g_settings) g_settings->is_fullscreen = g_isFullscreen;

    if (g_isFullscreen) {
        glfwGetWindowPos(window, &g_savedWindowX, &g_savedWindowY);
        glfwGetWindowSize(window, &g_savedWindowW, &g_savedWindowH);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
    } else {
        glfwSetWindowMonitor(window, nullptr, g_savedWindowX, g_savedWindowY, g_savedWindowW, g_savedWindowH, 0);
    }
}

static void framebufferSizeCallback(GLFWwindow* /*window*/, int width, int height) {
    g_windowWidth = width;
    g_windowHeight = height;
    glViewport(0, 0, width, height);
}

static void mouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (g_camera) {
        double x, y;
        glfwGetCursorPos(glfwGetCurrentContext(), &x, &y);
        g_camera->onMouseButton(button, action, x, y);
    }
}

static void cursorPosCallback(GLFWwindow* /*window*/, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (g_camera) {
        g_camera->onMouseMove(xpos, ypos);
    }
}

static void scrollCallback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (g_camera) {
        g_camera->onScroll(yoffset);
    }
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else if (key == GLFW_KEY_F11 || key == GLFW_KEY_F) {
            toggleFullscreen(window);
        } else if (key == GLFW_KEY_TAB || key == GLFW_KEY_H) {
            if (g_settings) g_settings->show_gui = !g_settings->show_gui;
        } else if (key == GLFW_KEY_R && g_camera) {
            g_camera->reset(45.0f);
        } else if (key == GLFW_KEY_SPACE && g_camera) {
            g_camera->autoRotate = !g_camera->autoRotate;
        } else if (key == GLFW_KEY_1 && g_settings && g_particleRenderer) {
            g_settings->mode = orbital::SimulationMode::ParticleCloud3D;
            g_particleRenderer->generateParticles(*g_settings);
        } else if (key == GLFW_KEY_2 && g_settings) {
            g_settings->mode = orbital::SimulationMode::VolumetricRaymarch;
        } else if (key == GLFW_KEY_3 && g_settings) {
            g_settings->mode = orbital::SimulationMode::QuantumExcitation;
            if (g_excitationRenderer) g_excitationRenderer->ensureInitialized(*g_settings);
        } else if (key == GLFW_KEY_4 && g_settings) {
            g_settings->mode = orbital::SimulationMode::WaveSlice2D;
            if (g_camera) { g_camera->azimuth = 0.0f; g_camera->elevation = float(M_PI) / 2.0f; }
        } else if (key == GLFW_KEY_5 && g_settings) {
            g_settings->mode = orbital::SimulationMode::BohrVsQuantum;
        }
    }
}

int main(int argc, char** argv) {
    std::cout << "========================================================\n";
    std::cout << "     Quantum Atom & Orbital Simulator (C++ & OpenGL)    \n";
    std::cout << "========================================================\n";
    std::cout << "Initializing GLFW...\n";

    if (!glfwInit()) {
        std::cerr << "CRITICAL: Failed to initialize GLFW!\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

    GLFWwindow* window = glfwCreateWindow(g_windowWidth, g_windowHeight,
                                          "Quantum Atom Simulator - Hydrogen Orbitals (OpenGL C++)",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "CRITICAL: Failed to create GLFW window!\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    // Callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "CRITICAL: Failed to initialize GLEW: " << glewGetErrorString(err) << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "GLSL Version:    " << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Initialize subsystems
    orbital::ArcballCamera camera;
    orbital::SimulationSettings settings;
    orbital::ParticleRenderer particleRenderer;
    orbital::RaytracerRenderer raytracerRenderer;
    orbital::ExcitationRenderer excitationRenderer;
    orbital::Wave2DRenderer wave2DRenderer;
    orbital::BohrRenderer bohrRenderer;
    orbital::Gui gui;

    g_camera = &camera;
    g_settings = &settings;
    g_particleRenderer = &particleRenderer;
    g_excitationRenderer = &excitationRenderer;

    if (!particleRenderer.init()) {
        std::cerr << "Failed to initialize ParticleRenderer!\n";
    }
    if (!raytracerRenderer.init()) {
        std::cerr << "Failed to initialize RaytracerRenderer!\n";
    }
    if (!excitationRenderer.init()) {
        std::cerr << "Failed to initialize ExcitationRenderer!\n";
    }
    if (!wave2DRenderer.init()) {
        std::cerr << "Failed to initialize Wave2DRenderer!\n";
    }
    if (!bohrRenderer.init()) {
        std::cerr << "Failed to initialize BohrRenderer!\n";
    }
    if (!gui.init(window)) {
        std::cerr << "Failed to initialize GUI!\n";
    }

    // Initialize default states
    settings.setOrbital(2, 1, 0);
    particleRenderer.generateParticles(settings);
    excitationRenderer.ensureInitialized(settings);

    auto lastTime = std::chrono::high_resolution_clock::now();
    float fps = 60.0f;
    float fpsTimer = 0.0f;
    int frameCounter = 0;

    std::cout << "\nReady! Controls:\n"
              << "  [F11] or [F]: Toggle Fullscreen\n"
              << "  [TAB] or [H]: Toggle / Hide Control Panel (100% Unobstructed Fullscreen)\n"
              << "  [1-5]: Switch Simulation Modes\n"
              << "  [R]: Reset Camera Orbit\n"
              << "  [Space]: Toggle Auto-Rotation\n"
              << "  [Left Drag]: Orbit Camera\n"
              << "  [Right Drag]: Pan Camera\n"
              << "  [Scroll]: Zoom Camera\n"
              << "  [ESC]: Exit\n\n";

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        deltaTime = std::min(deltaTime, 0.1f); // Clamp delta

        frameCounter++;
        fpsTimer += deltaTime;
        if (fpsTimer >= 0.5f) {
            fps = float(frameCounter) / fpsTimer;
            frameCounter = 0;
            fpsTimer = 0.0f;
        }

        glfwPollEvents();

        // Update systems
        camera.update(deltaTime);

        switch (settings.mode) {
            case orbital::SimulationMode::ParticleCloud3D:
                particleRenderer.update(deltaTime, settings);
                break;
            case orbital::SimulationMode::QuantumExcitation:
                excitationRenderer.update(deltaTime, settings);
                break;
            case orbital::SimulationMode::BohrVsQuantum:
                bohrRenderer.update(deltaTime, settings);
                break;
            default:
                break;
        }

        // Render pass
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        // Deep space cosmic dark background
        glClearColor(0.03f, 0.04f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render active simulation mode
        switch (settings.mode) {
            case orbital::SimulationMode::ParticleCloud3D:
                particleRenderer.render(camera, settings, fbWidth, fbHeight);
                break;
            case orbital::SimulationMode::VolumetricRaymarch:
                raytracerRenderer.render(camera, settings, fbWidth, fbHeight);
                break;
            case orbital::SimulationMode::QuantumExcitation:
                excitationRenderer.render(camera, settings, fbWidth, fbHeight);
                break;
            case orbital::SimulationMode::WaveSlice2D:
                wave2DRenderer.render(camera, settings, fbWidth, fbHeight);
                break;
            case orbital::SimulationMode::BohrVsQuantum:
                bohrRenderer.render(camera, settings, fbWidth, fbHeight);
                if (settings.bohr_show_cloud_overlay) {
                    // Overlay quantum probability cloud
                    particleRenderer.render(camera, settings, fbWidth, fbHeight);
                }
                break;
        }

        // GUI Pass
        gui.newFrame();
        gui.render(settings, camera, particleRenderer, excitationRenderer, fps, window);
        gui.draw();

        glfwSwapBuffers(window);
    }

    // Cleanup
    gui.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::cout << "Exited successfully.\n";
    return 0;
}
