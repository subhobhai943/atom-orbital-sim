#ifndef ATOM_STATE_H
#define ATOM_STATE_H

#include "orbital_math.h"
#include "color_maps.h"
#include <random>
#include <vector>

namespace orbital {

enum class SimulationMode {
    ParticleCloud3D = 0,     // Real-time 3D Monte Carlo point cloud with probability current
    VolumetricRaymarch = 1,  // Analytical wavefunction GLSL volumetric raymarcher
    QuantumExcitation = 2,   // Photon emission/absorption quantum jump transition
    WaveSlice2D = 3,         // 2D cross-section contour & phase lobe visualizer
    BohrVsQuantum = 4        // 3D Bohr planetary model vs quantum probability comparison
};

struct SimulationSettings {
    // Current active mode
    SimulationMode mode = SimulationMode::ParticleCloud3D;

    // Quantum numbers
    int n = 2; // Principal (1, 2, 3, 4, 5...)
    int l = 1; // Azimuthal (0 <= l < n)
    int m = 0; // Magnetic (-l <= m <= l)
    bool use_real_orbitals = true; // Use chemistry representation (px, py, pz, d...)

    // Particle Cloud settings
    int particle_count = 120000;
    float point_base_size = 3.5f;
    float brightness = 1.6f;
    ColorPalette palette = ColorPalette::Fire;
    bool enable_probability_flow = true; // Animate azimuthal current v_phi for m != 0
    float flow_speed = 0.6f;

    // Slicing plane to inspect internal nodal surfaces
    bool enable_slice = false;
    float slice_height = 0.0f; // Slices along Y axis (-20 to +20)
    int slice_axis = 1;        // 0 = X, 1 = Y, 2 = Z

    // Nucleus visualization
    bool show_nucleus = true;
    float nucleus_scale = 1.0f;

    // Volumetric Raymarcher settings
    int ray_steps = 96;
    float volume_density = 2.5f;
    float volume_bound = 30.0f;

    // 2D Wave Slice settings
    int slice_resolution = 300;
    float slice_extent = 35.0f;
    bool show_contours = true;
    bool show_nodal_lines = true;

    // Quantum Excitation / Transition state
    int exc_n1 = 1;
    int exc_l1 = 0;
    int exc_m1 = 0;
    int exc_n2 = 3;
    int exc_l2 = 1;
    int exc_m2 = 0;
    float transition_progress = 0.0f; // 0.0 = state 1, 1.0 = state 2
    bool transition_animating = false;
    float transition_speed = 0.5f;    // transition duration factor
    bool photon_incoming = false;
    bool photon_outgoing = false;
    Vec3 photon_pos{0.0f, 0.0f, 0.0f};
    Vec3 photon_dir{1.0f, 0.0f, 0.0f};
    double photon_wavelength = 0.0;
    Color4 photon_color{1.0f, 1.0f, 1.0f, 1.0f};

    // Bohr model state
    float bohr_orbit_angle = 0.0f;
    int bohr_n = 2;
    bool bohr_show_wave = true; // Show de Broglie standing wave on orbit
    bool bohr_show_cloud_overlay = true; // Show quantum cloud ghost overlay

    // Math samplers & random generator
    std::mt19937 rng{1337};
    RadialSampler radial_sampler;
    ThetaSampler theta_sampler;

    void clampQuantumNumbers() {
        if (n < 1) n = 1;
        if (n > 7) n = 7;
        if (l < 0) l = 0;
        if (l >= n) l = n - 1;
        if (m > l) m = l;
        if (m < -l) m = -l;
    }

    void setOrbital(int new_n, int new_l, int new_m) {
        n = new_n;
        l = new_l;
        m = new_m;
        clampQuantumNumbers();
    }
};

} // namespace orbital

#endif // ATOM_STATE_H
