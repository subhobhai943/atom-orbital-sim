#include "../include/gui.h"
#include "../include/renderer_particles.h"
#include "../include/renderer_excitation.h"
#include "../third_party/imgui/imgui.h"
#include "../third_party/imgui/backends/imgui_impl_glfw.h"
#include "../third_party/imgui/backends/imgui_impl_opengl3.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace orbital {

bool Gui::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Dark sleek scientific theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.5f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.12f, 0.92f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.35f, 0.70f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.36f, 0.50f, 0.85f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.45f, 0.65f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.24f, 0.35f, 0.80f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.38f, 0.58f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.50f, 0.75f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.14f, 0.18f, 0.85f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.18f, 0.22f, 0.28f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.25f, 0.65f, 0.95f, 0.90f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.16f, 0.24f, 1.00f);

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        std::cerr << "Failed to init ImGui GLFW!\n";
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
        std::cerr << "Failed to init ImGui OpenGL3!\n";
        return false;
    }

    return true;
}

void Gui::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

static void toggleFullscreenHelper(GLFWwindow* window, orbital::SimulationSettings& settings) {
    static int savedX = 100, savedY = 100, savedW = 1280, savedH = 800;
    settings.is_fullscreen = !settings.is_fullscreen;
    if (settings.is_fullscreen) {
        glfwGetWindowPos(window, &savedX, &savedY);
        glfwGetWindowSize(window, &savedW, &savedH);
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (monitor) {
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
    } else {
        glfwSetWindowMonitor(window, nullptr, savedX, savedY, savedW, savedH, 0);
    }
}

void Gui::renderHiddenHudOverlay(SimulationSettings& settings, float fps) {
    ImGuiIO& io = ImGui::GetIO();

    // 1. Top-Left Floating Action Buttons
    ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.40f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("##HiddenPill", nullptr, flags)) {
        if (ImGui::Button(" ⚙ Show Controls (TAB) ")) {
            settings.show_gui = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(settings.is_fullscreen ? " 🗗 Windowed " : " ⛶ Fullscreen (F11) ")) {
            toggleFullscreenHelper(glfwGetCurrentContext(), settings);
        }
    }
    ImGui::End();

    // 2. Bottom Status Bar Overlay
    ImGui::SetNextWindowPos(ImVec2(15, io.DisplaySize.y - 42), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("##BottomBadge", nullptr, flags)) {
        std::string name = orbital_name(settings.n, settings.l, settings.m, settings.use_real_orbitals);
        double energy = energy_level_ev(settings.n);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "⚛ %s", name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("|  En: %.2f eV  |  FPS: %.0f  |  [TAB/H] Controls  |  [F11] Fullscreen  |  [1-5] Modes  |  [Space] Auto-Orbit", energy, fps);
    }
    ImGui::End();
}

void Gui::renderMiniHud(SimulationSettings& settings, ParticleRenderer& particleRenderer, ExcitationRenderer& excitationRenderer, GLFWwindow* window, float fps) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 46), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.60f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("##TopMiniHud", nullptr, flags)) {
        std::string name = orbital_name(settings.n, settings.l, settings.m, settings.use_real_orbitals);
        double energy = energy_level_ev(settings.n);
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "⚛ %s (%.2feV)", name.c_str(), energy);
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Mode buttons
        const char* modeLabels[] = {"1.Cloud", "2.Raymarch", "3.Jumps", "4.2D Slice", "5.Bohr"};
        for (int i = 0; i < 5; ++i) {
            bool active = (static_cast<int>(settings.mode) == i);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.90f, 0.9f));
            if (ImGui::Button(modeLabels[i])) {
                settings.mode = static_cast<SimulationMode>(i);
                if (settings.mode == SimulationMode::ParticleCloud3D) {
                    particleRenderer.generateParticles(settings);
                } else if (settings.mode == SimulationMode::QuantumExcitation) {
                    excitationRenderer.ensureInitialized(settings);
                }
            }
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine();
        }

        ImGui::TextDisabled("| Presets:");
        ImGui::SameLine();
        auto miniPreset = [&](int n, int l, int m, const char* label) {
            if (ImGui::Button(label)) {
                settings.setOrbital(n, l, m);
                if (settings.mode == SimulationMode::ParticleCloud3D) {
                    particleRenderer.generateParticles(settings);
                }
            }
            ImGui::SameLine();
        };
        miniPreset(1, 0, 0, "1s");
        miniPreset(2, 0, 0, "2s");
        miniPreset(2, 1, 0, "2pz");
        miniPreset(3, 2, 0, "3dz²");

        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (ImGui::Button("⚙ Full Panel")) {
            settings.mini_hud_mode = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(settings.is_fullscreen ? "🗗" : "⛶ Fullscreen (F11)")) {
            toggleFullscreenHelper(window, settings);
        }
        ImGui::SameLine();
        if (ImGui::Button("✕ Hide (TAB)")) {
            settings.show_gui = false;
        }
    }
    ImGui::End();
}

void Gui::render(SimulationSettings& settings,
                 ArcballCamera& camera,
                 ParticleRenderer& particleRenderer,
                 ExcitationRenderer& excitationRenderer,
                 float fps,
                 GLFWwindow* window) {
    if (!settings.show_gui) {
        renderHiddenHudOverlay(settings, fps);
        return;
    }

    if (settings.mini_hud_mode) {
        renderMiniHud(settings, particleRenderer, excitationRenderer, window, fps);
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(15, 15), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(410, 740), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Quantum Hydrogen Atom Simulator", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Quick Action Bar: Fullscreen, Mini-HUD, Hide
        if (ImGui::Button(settings.is_fullscreen ? " 🗗 Windowed " : " ⛶ Fullscreen (F11) ")) {
            toggleFullscreenHelper(window, settings);
        }
        ImGui::SameLine();
        if (ImGui::Button(" ▾ Mini-Bar HUD ")) {
            settings.mini_hud_mode = true;
        }
        ImGui::SameLine();
        if (ImGui::Button(" ✕ Hide Panel (TAB) ")) {
            settings.show_gui = false;
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "Schrodinger Orbital Visualizer");
        ImGui::TextDisabled("FPS: %.1f | Particles: %d", fps, particleRenderer.getActualCount());
        ImGui::Separator();

        renderModeSelection(settings, particleRenderer, excitationRenderer);

        if (ImGui::CollapsingHeader("Quantum Numbers (n, l, m)", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderOrbitalControls(settings, particleRenderer);
        }

        if (ImGui::CollapsingHeader("Orbital Presets", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderPresets(settings, particleRenderer);
        }

        if (ImGui::CollapsingHeader("Visual & Rendering Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
            renderVisualSettings(settings, particleRenderer);
        }

        if (settings.mode == SimulationMode::QuantumExcitation || ImGui::CollapsingHeader("Quantum Jumps & Excitation")) {
            renderExcitationControls(settings, excitationRenderer);
        }

        if (ImGui::CollapsingHeader("Quantum Physics Telemetry")) {
            renderPhysicsTelemetry(settings);
        }

        if (ImGui::CollapsingHeader("Camera & View")) {
            renderCameraControls(camera);
        }
    }
    ImGui::End();
}

void Gui::renderModeSelection(SimulationSettings& settings, ParticleRenderer& particleRenderer, ExcitationRenderer& excitationRenderer) {
    ImGui::Text("Simulation Mode:");
    const char* modes[] = {
        "1. 3D Electron Cloud (Point Cloud)",
        "2. 3D Volumetric Raymarcher (GPU)",
        "3. Quantum Transitions & Jumps",
        "4. 2D Wavefunction Slice & Nodes",
        "5. Bohr Model vs Quantum Mechanics"
    };

    int currentMode = static_cast<int>(settings.mode);
    if (ImGui::Combo("##ModeCombo", &currentMode, modes, IM_ARRAYSIZE(modes))) {
        settings.mode = static_cast<SimulationMode>(currentMode);
        if (settings.mode == SimulationMode::ParticleCloud3D) {
            particleRenderer.generateParticles(settings);
        } else if (settings.mode == SimulationMode::QuantumExcitation) {
            excitationRenderer.ensureInitialized(settings);
        }
    }
    ImGui::Spacing();
}

void Gui::renderOrbitalControls(SimulationSettings& settings, ParticleRenderer& particleRenderer) {
    bool changed = false;

    // Principal Quantum Number n: 1 .. 6
    if (ImGui::SliderInt("n (Principal)", &settings.n, 1, 6)) {
        settings.clampQuantumNumbers();
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Principal quantum number n: governs orbital energy and radial extent.");

    // Azimuthal Quantum Number l: 0 .. n-1
    if (ImGui::SliderInt("l (Azimuthal)", &settings.l, 0, settings.n - 1)) {
        settings.clampQuantumNumbers();
        changed = true;
    }
    const char subshell_names[] = {'s', 'p', 'd', 'f', 'g', 'h'};
    char sub = (settings.l >= 0 && settings.l < 6) ? subshell_names[settings.l] : '?';
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f), "(%c)", sub);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Orbital angular momentum l: orbital geometric shape (s=sphere, p=dumbbell, d=cloverleaf, f=octupole).");

    // Magnetic Quantum Number m: -l .. +l
    if (ImGui::SliderInt("m (Magnetic)", &settings.m, -settings.l, settings.l)) {
        settings.clampQuantumNumbers();
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Magnetic quantum number m: spatial orientation and projected angular momentum Lz = m * hbar.");

    // Chemistry Real Orbitals vs Complex Eigenstates
    if (ImGui::Checkbox("Real Chemistry Orbitals (px, py, pz, d...)", &settings.use_real_orbitals)) {
        changed = true;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Real linear combinations (directional lobes) vs complex eigenstates (uniform phi with azimuthal current flow).");

    std::string name = orbital_name(settings.n, settings.l, settings.m, settings.use_real_orbitals);
    ImGui::Text("Active Orbital: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "%s", name.c_str());

    if (changed && settings.mode == SimulationMode::ParticleCloud3D) {
        particleRenderer.generateParticles(settings);
    }
}

void Gui::renderPresets(SimulationSettings& settings, ParticleRenderer& particleRenderer) {
    auto applyPreset = [&](int n, int l, int m, const char* /*label*/) {
        settings.setOrbital(n, l, m);
        if (settings.mode == SimulationMode::ParticleCloud3D) {
            particleRenderer.generateParticles(settings);
        }
    };

    if (ImGui::Button("1s Ground"))        applyPreset(1, 0, 0, "1s");
    ImGui::SameLine();
    if (ImGui::Button("2s Shell"))         applyPreset(2, 0, 0, "2s");
    ImGui::SameLine();
    if (ImGui::Button("2pz Dumbbell"))     applyPreset(2, 1, 0, "2pz");
    ImGui::SameLine();
    if (ImGui::Button("2px Lobe"))         applyPreset(2, 1, 1, "2px");

    if (ImGui::Button("3dz² Donut"))       applyPreset(3, 2, 0, "3dz2");
    ImGui::SameLine();
    if (ImGui::Button("3dxy Clover"))      applyPreset(3, 2, -2, "3dxy");
    ImGui::SameLine();
    if (ImGui::Button("4fz³ Flower"))      applyPreset(4, 3, 0, "4fz3");
}

void Gui::renderVisualSettings(SimulationSettings& settings, ParticleRenderer& particleRenderer) {
    // Particle count
    bool regenerate = false;
    if (ImGui::SliderInt("Particle Count", &settings.particle_count, 10000, 350000, "%d")) {
        // regenerate on release or button
    }
    if (ImGui::Button("Apply & Resample Points")) {
        regenerate = true;
    }

    ImGui::SliderFloat("Point Size", &settings.point_base_size, 1.0f, 12.0f, "%.1f");
    ImGui::SliderFloat("Glow Brightness", &settings.brightness, 0.2f, 4.0f, "%.2f");

    // Color Palette
    const char* palettes[] = {
        "Thermal Fire / Blackbody",
        "Neon Cyan / Quantum Electric",
        "Quantum Phase (+Red / -Blue)",
        "Plasma / Synthwave",
        "Deep Cosmic Nebula"
    };
    int currentPal = static_cast<int>(settings.palette);
    if (ImGui::Combo("Palette", &currentPal, palettes, IM_ARRAYSIZE(palettes))) {
        settings.palette = static_cast<ColorPalette>(currentPal);
        regenerate = true;
    }

    // Probability flow
    ImGui::Checkbox("Probability Current Flow (v_phi)", &settings.enable_probability_flow);
    if (settings.enable_probability_flow) {
        ImGui::SliderFloat("Flow Speed", &settings.flow_speed, 0.1f, 3.0f, "%.2f");
    }

    // Slicing plane
    ImGui::Checkbox("Cross-Section Slicing Plane", &settings.enable_slice);
    if (settings.enable_slice) {
        ImGui::RadioButton("X-Axis", &settings.slice_axis, 0); ImGui::SameLine();
        ImGui::RadioButton("Y-Axis", &settings.slice_axis, 1); ImGui::SameLine();
        ImGui::RadioButton("Z-Axis", &settings.slice_axis, 2);
        ImGui::SliderFloat("Slice Offset", &settings.slice_height, -30.0f, 30.0f, "%.1f");
    }

    // Nucleus
    ImGui::Checkbox("Show Nucleus", &settings.show_nucleus);
    if (settings.show_nucleus) {
        ImGui::SliderFloat("Nucleus Scale", &settings.nucleus_scale, 0.2f, 3.0f, "%.2f");
    }

    // Volumetric mode parameters
    if (settings.mode == SimulationMode::VolumetricRaymarch) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Raymarch Parameters:");
        ImGui::SliderInt("Ray Steps", &settings.ray_steps, 32, 128);
        ImGui::SliderFloat("Volume Density", &settings.volume_density, 0.5f, 8.0f, "%.2f");
    }

    // 2D slice parameters
    if (settings.mode == SimulationMode::WaveSlice2D) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "2D Slice Parameters:");
        ImGui::Checkbox("Iso-Probability Contours", &settings.show_contours);
        ImGui::Checkbox("Nodal Boundary Lines", &settings.show_nodal_lines);
    }

    // Bohr comparison parameters
    if (settings.mode == SimulationMode::BohrVsQuantum) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Bohr Parameters:");
        ImGui::SliderInt("Bohr Shell (n)", &settings.bohr_n, 1, 4);
        ImGui::Checkbox("de Broglie Matter Wave", &settings.bohr_show_wave);
    }

    if (regenerate && settings.mode == SimulationMode::ParticleCloud3D) {
        particleRenderer.generateParticles(settings);
    }
}

void Gui::renderExcitationControls(SimulationSettings& settings, ExcitationRenderer& excitationRenderer) {
    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Quantum Jump Simulator");

    double lambda = photon_wavelength_nm(settings.exc_n1, settings.exc_n2);
    double dE = transition_energy_ev(settings.exc_n1, settings.exc_n2);
    int lower_n = std::min(settings.exc_n1, settings.exc_n2);
    std::string series = excitationRenderer.getSpectralSeriesName(lower_n);

    ImGui::Text("Transition: %d -> %d", settings.exc_n1, settings.exc_n2);
    ImGui::Text("Energy Delta: %.3f eV", dE);
    ImGui::Text("Photon Wavelength: %.1f nm", lambda);
    ImGui::Text("Series: %s", series.c_str());

    Color4 col = wavelength_to_rgb(lambda);
    ImGui::Text("Spectral Color: ");
    ImGui::SameLine();
    ImGui::ColorButton("##PhotonCol", ImVec4(col.r, col.g, col.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(40, 16));

    ImGui::SliderFloat("Transition Speed", &settings.transition_speed, 0.1f, 2.0f, "%.2f");

    if (ImGui::Button("Absorb Lyman-alpha (1s -> 2p, 121nm UV)")) {
        settings.mode = SimulationMode::QuantumExcitation;
        settings.setOrbital(1, 0, 0);
        excitationRenderer.triggerTransition(settings, 2, 1, 0);
    }
    if (ImGui::Button("Emit Balmer H-alpha (3p -> 2s, 656nm Red)")) {
        settings.mode = SimulationMode::QuantumExcitation;
        settings.setOrbital(3, 1, 0);
        excitationRenderer.triggerTransition(settings, 2, 0, 0);
    }
    if (ImGui::Button("Emit Balmer H-beta (4p -> 2s, 486nm Cyan)")) {
        settings.mode = SimulationMode::QuantumExcitation;
        settings.setOrbital(4, 1, 0);
        excitationRenderer.triggerTransition(settings, 2, 0, 0);
    }
    if (ImGui::Button("Emit Balmer H-gamma (5p -> 2s, 434nm Blue)")) {
        settings.mode = SimulationMode::QuantumExcitation;
        settings.setOrbital(5, 1, 0);
        excitationRenderer.triggerTransition(settings, 2, 0, 0);
    }
    if (ImGui::Button("Emit Paschen-alpha (4d -> 3p, 1875nm IR)")) {
        settings.mode = SimulationMode::QuantumExcitation;
        settings.setOrbital(4, 2, 0);
        excitationRenderer.triggerTransition(settings, 3, 1, 0);
    }

    if (settings.transition_animating) {
        ImGui::ProgressBar(settings.transition_progress, ImVec2(-1, 0), "Quantum Jump in Progress...");
    } else {
        ImGui::TextDisabled("Status: Atom in stable eigenstate.");
    }
}

void Gui::renderPhysicsTelemetry(const SimulationSettings& settings) {
    double energy = energy_level_ev(settings.n);
    int radial_nodes = settings.n - settings.l - 1;
    int angular_nodes = settings.l;
    int total_nodes = settings.n - 1;
    double l_mag = std::sqrt(static_cast<double>(settings.l * (settings.l + 1)));
    double mean_r = 0.5 * (3.0 * settings.n * settings.n - settings.l * (settings.l + 1));

    ImGui::BulletText("Energy Level En: %.3f eV", energy);
    ImGui::BulletText("Bohr Radius rn = n² * a0: %d a0", settings.n * settings.n);
    ImGui::BulletText("Mean Radius <r>: %.1f a0", mean_r);
    ImGui::BulletText("Angular Momentum |L|: %.2f hbar", l_mag);
    ImGui::BulletText("Z-component Lz: %d hbar", settings.m);
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.9f, 1.0f), "Nodal Geometry:");
    ImGui::BulletText("Radial Nodes (Spheres): %d", radial_nodes);
    ImGui::BulletText("Angular Nodes (Cones/Planes): %d", angular_nodes);
    ImGui::BulletText("Total Nodal Surfaces: %d", total_nodes);
}

void Gui::renderCameraControls(ArcballCamera& camera) {
    if (ImGui::Button("Reset Camera Orbit")) {
        camera.reset(45.0f);
    }
    ImGui::Checkbox("Auto-Rotate View", &camera.autoRotate);
    if (camera.autoRotate) {
        ImGui::SliderFloat("Rotation Speed", &camera.autoRotateSpeed, 0.05f, 1.0f, "%.2f rad/s");
    }
    ImGui::SliderFloat("FOV", &camera.fov, 25.0f, 90.0f, "%.1f deg");
    ImGui::TextDisabled("Controls: Left Drag to Orbit, Right Drag to Pan, Scroll to Zoom.");
}

void Gui::draw() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

} // namespace orbital
