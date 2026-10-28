#ifndef COLOR_MAPS_H
#define COLOR_MAPS_H

#include "orbital_math.h"
#include <algorithm>
#include <cmath>

namespace orbital {

enum class ColorPalette {
    Fire = 0,
    NeonCyan = 1,
    QuantumPhase = 2,
    Plasma = 3,
    GhostNebula = 4
};

inline const char* get_palette_name(ColorPalette palette) {
    switch (palette) {
        case ColorPalette::Fire: return "Thermal Fire / Blackbody";
        case ColorPalette::NeonCyan: return "Neon Cyan / Electric";
        case ColorPalette::QuantumPhase: return "Quantum Phase (+Red / -Blue)";
        case ColorPalette::Plasma: return "Plasma / Synthwave";
        case ColorPalette::GhostNebula: return "Deep Cosmic Nebula";
        default: return "Unknown";
    }
}

// Linear color ramp sampler from an array of key color stops
template <size_t N>
inline Color4 sample_gradient(float t, const Color4 (&stops)[N]) {
    t = std::clamp(t, 0.0f, 1.0f);
    float scaled = t * (N - 1);
    int i = static_cast<int>(scaled);
    int next = std::min(i + 1, static_cast<int>(N - 1));
    float f = scaled - i;

    return {
        stops[i].r + f * (stops[next].r - stops[i].r),
        stops[i].g + f * (stops[next].g - stops[i].g),
        stops[i].b + f * (stops[next].b - stops[i].b),
        stops[i].a + f * (stops[next].a - stops[i].a)
    };
}

/// Palette 0: Thermal Fire (Kavan G standard)
inline Color4 palette_fire(float t) {
    const Color4 stops[6] = {
        {0.02f, 0.01f, 0.05f, 0.2f}, // 0.0 Deep violet black
        {0.32f, 0.02f, 0.58f, 0.6f}, // 0.2 Dark Purple
        {0.85f, 0.05f, 0.05f, 0.85f},// 0.4 Crimson Red
        {1.00f, 0.50f, 0.00f, 0.95f},// 0.6 Bright Orange
        {1.00f, 0.95f, 0.20f, 1.0f}, // 0.8 Luminous Yellow
        {1.00f, 1.00f, 1.00f, 1.0f}  // 1.0 White hot
    };
    return sample_gradient(t, stops);
}

/// Palette 1: Neon Cyan / Quantum Electric
inline Color4 palette_neon_cyan(float t) {
    const Color4 stops[5] = {
        {0.01f, 0.04f, 0.12f, 0.2f},
        {0.05f, 0.25f, 0.70f, 0.65f},
        {0.00f, 0.75f, 0.95f, 0.85f},
        {0.40f, 0.95f, 1.00f, 0.95f},
        {1.00f, 1.00f, 1.00f, 1.0f}
    };
    return sample_gradient(t, stops);
}

/// Palette 2: Quantum Phase (+ lobe is warm reddish-orange, - lobe is cool cyan-blue, nodal boundary is subtle)
inline Color4 palette_phase(float psi_val, float max_psi) {
    if (std::abs(max_psi) < 1e-8f) return {0.5f, 0.5f, 0.5f, 0.2f};
    float norm = std::clamp(psi_val / max_psi, -1.0f, 1.0f);
    float magnitude = std::abs(norm);

    if (norm >= 0.0f) {
        // Positive phase: orange-red gradient
        return {
            1.0f,
            0.2f + 0.6f * magnitude,
            0.1f * (1.0f - magnitude),
            std::clamp(0.2f + 0.8f * magnitude, 0.0f, 1.0f)
        };
    } else {
        // Negative phase: cyan-blue gradient
        return {
            0.1f * (1.0f - magnitude),
            0.4f + 0.5f * magnitude,
            1.0f,
            std::clamp(0.2f + 0.8f * magnitude, 0.0f, 1.0f)
        };
    }
}

/// Palette 3: Plasma / Synthwave
inline Color4 palette_plasma(float t) {
    const Color4 stops[5] = {
        {0.05f, 0.01f, 0.15f, 0.2f},
        {0.55f, 0.00f, 0.65f, 0.7f},
        {0.90f, 0.20f, 0.45f, 0.85f},
        {1.00f, 0.65f, 0.10f, 0.95f},
        {1.00f, 0.98f, 0.60f, 1.0f}
    };
    return sample_gradient(t, stops);
}

/// Palette 4: Cosmic Ghost Nebula
inline Color4 palette_ghost_nebula(float t) {
    const Color4 stops[5] = {
        {0.02f, 0.02f, 0.08f, 0.2f},
        {0.15f, 0.35f, 0.55f, 0.6f},
        {0.20f, 0.75f, 0.65f, 0.85f},
        {0.65f, 0.95f, 0.85f, 0.95f},
        {1.00f, 1.00f, 1.00f, 1.0f}
    };
    return sample_gradient(t, stops);
}

inline Color4 map_orbital_color(float intensity, ColorPalette palette) {
    switch (palette) {
        case ColorPalette::Fire: return palette_fire(intensity);
        case ColorPalette::NeonCyan: return palette_neon_cyan(intensity);
        case ColorPalette::Plasma: return palette_plasma(intensity);
        case ColorPalette::GhostNebula: return palette_ghost_nebula(intensity);
        case ColorPalette::QuantumPhase: return palette_fire(intensity);
        default: return palette_fire(intensity);
    }
}

} // namespace orbital

#endif // COLOR_MAPS_H
