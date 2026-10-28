// =============================================================================
// launcher.cpp — Interactive Terminal Launcher for Simulation Modes
// =============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

static std::string build_command(const std::string& base_name) {
    fs::path p_cwd = fs::current_path() / base_name;
    fs::path p_bin = fs::current_path() / "bin" / base_name;
    fs::path p_build = fs::current_path() / "build" / base_name;

    std::string target = base_name;
    if (fs::exists(p_bin)) target = p_bin.string();
    else if (fs::exists(p_build)) target = p_build.string();
    else if (fs::exists(p_cwd)) target = p_cwd.string();

    return target;
}

static void launch(const std::string& base_name) {
    std::string cmd = build_command(base_name);
    std::cout << "\n>>> Launching: " << cmd << " ...\n";
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Process exited with code: " << ret << "\n";
    }
}

int main() {
    const std::vector<std::pair<std::string, std::string>> options = {
        {"Unified Quantum Atom Simulator (ImGui Master App)", "atom_orbital_sim"},
        {"Realtime 3D Quantum Cloud (Interactive Points)",    "atom_realtime"},
        {"GPU Volumetric Wavefunction Raymarcher",             "atom_raytracer"},
        {"Quantum Excitation & Photon Jumps (Balmer/Lyman)",  "atom_excitation"},
        {"2D Wavefunction Slice & Nodal Boundary",            "wave_atom_2d"},
        {"3D Bohr Planetary Model vs Quantum Mechanics",      "atom_bohr"},
        {"Run Quantum Math Unit Tests",                       "test_orbital_math"}
    };

    while (true) {
        std::cout << "\n=======================================================\n";
        std::cout << "        Hydrogen Quantum Orbital Visualizer            \n";
        std::cout << "=======================================================\n\n";

        for (size_t i = 0; i < options.size(); ++i) {
            std::cout << "  [" << (i + 1) << "] " << options[i].first << "\n";
        }
        std::cout << "  [Q] Quit\n";
        std::cout << "\nSelect an option: ";

        std::string input;
        if (!(std::cin >> input)) break;

        if (input == "q" || input == "Q") break;

        try {
            int sel = std::stoi(input);
            if (sel >= 1 && sel <= static_cast<int>(options.size())) {
                launch(options[sel - 1].second);
            } else {
                std::cout << "Invalid selection! Try again.\n";
            }
        } catch (...) {
            std::cout << "Please enter a valid number.\n";
        }
    }

    return 0;
}
