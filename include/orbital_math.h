#ifndef ORBITAL_MATH_H
#define ORBITAL_MATH_H

// =============================================================================
// orbital_math.h — Hydrogen Quantum Mechanics & Orbital Sampling Mathematics
// =============================================================================
//
// Analytical solution to the Time-Independent Schrödinger Equation for
// hydrogen-like atoms in spherical coordinates (r, θ, φ):
//
//   ψ_{n,l,m}(r, θ, φ) = R_{nl}(r) · Y_l^m(θ, φ)
//
// Where:
//   - n : Principal quantum number (1, 2, 3, ...) -> Energy level & size
//   - l : Azimuthal / Orbital angular momentum (0 <= l < n) -> Shape (s, p, d, f)
//   - m : Magnetic quantum number (-l <= m <= l) -> Spatial orientation
//
// Wavefunction components:
//   - R_{nl}(r): Associated Laguerre polynomials L_{n-l-1}^{2l+1}(2r / n*a0)
//   - Y_l^m(θ, φ): Associated Legendre polynomials P_l^{|m|}(cos θ) and e^{i*m*φ}
//
// Also supports Real Orbitals (Cartesian linear combinations like px, py, pz,
// dxy, dx2-y2, dz2) commonly used in chemistry and physical rendering.
//
// =============================================================================

#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <complex>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace orbital {

constexpr double BOHR_RADIUS_A0 = 1.0;     // Atomic units (a0 = 1)
constexpr double RYDBERG_EV = 13.605693122994; // Ground state ionization energy (eV)
constexpr double HC_EV_NM = 1239.84193;    // Planck constant * speed of light (eV · nm)

// -----------------------------------------------------------------------------
// Basic 3D vector and 4D color structures
// -----------------------------------------------------------------------------
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }

    float length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        float l = length();
        return l > 1e-8f ? (*this / l) : Vec3(0, 0, 0);
    }
};

struct Color4 {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    Color4() = default;
    Color4(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}
};

// =============================================================================
// Special Functions
// =============================================================================

/// Generalized / Associated Laguerre polynomial L_k^{alpha}(x)
/// Recurrence relation:
///   L_0^a(x) = 1
///   L_1^a(x) = 1 + a - x
///   j * L_j^a(x) = (2j - 1 + a - x) * L_{j-1}^a(x) - (j - 1 + a) * L_{j-2}^a(x)
inline double assoc_laguerre(int k, int alpha, double x) {
    if (k < 0) return 0.0;
    if (k == 0) return 1.0;

    double Lm2 = 1.0;
    double Lm1 = 1.0 + alpha - x;
    if (k == 1) return Lm1;

    double L = Lm1;
    for (int j = 2; j <= k; ++j) {
        L = ((2 * j - 1 + alpha - x) * Lm1 - (j - 1 + alpha) * Lm2) / j;
        Lm2 = Lm1;
        Lm1 = L;
    }
    return L;
}

/// Associated Legendre polynomial P_l^m(x) with Condon-Shortley phase.
/// Requires 0 <= |m| <= l and x in [-1, 1].
inline double assoc_legendre(int l, int m, double x) {
    m = std::abs(m);
    if (m > l) return 0.0;
    x = std::clamp(x, -1.0, 1.0);

    // Compute P_m^m(x) = (-1)^m * (2m - 1)!! * (1 - x^2)^(m/2)
    double Pmm = 1.0;
    if (m > 0) {
        double somx2 = std::sqrt(std::max(0.0, (1.0 - x) * (1.0 + x)));
        double fact = 1.0;
        for (int i = 1; i <= m; ++i) {
            Pmm *= -fact * somx2;
            fact += 2.0;
        }
    }
    if (l == m) return Pmm;

    // Compute P_{m+1}^m(x) = x * (2m + 1) * P_m^m(x)
    double Pm1m = x * (2 * m + 1) * Pmm;
    if (l == m + 1) return Pm1m;

    // Recurrence for l >= m + 2
    double Plm = Pm1m;
    for (int ll = m + 2; ll <= l; ++ll) {
        Plm = ((2 * ll - 1) * x * Pm1m - (ll + m - 1) * Pmm) / (ll - m);
        Pmm = Pm1m;
        Pm1m = Plm;
    }
    return Plm;
}

// Log factorial helper to compute large factorials without overflow
inline double log_factorial(int n) {
    if (n <= 1) return 0.0;
    return std::lgamma(n + 1.0);
}

// =============================================================================
// Hydrogen Radial Wavefunction R_{nl}(r)
// =============================================================================
//
// R_{nl}(r) = sqrt( (2/(n*a0))^3 * (n-l-1)! / (2n * (n+l)!) )
//             * exp(-rho/2) * rho^l * L_{n-l-1}^{2l+1}(rho)
// where rho = 2r / (n * a0).
//
inline double radial_wavefunction(double r, int n, int l, double a0 = BOHR_RADIUS_A0) {
    if (r < 0.0 || n < 1 || l < 0 || l >= n) return 0.0;

    double rho = 2.0 * r / (n * a0);
    int k = n - l - 1;
    int alpha = 2 * l + 1;

    double log_norm = 3.0 * std::log(2.0 / (n * a0))
                    + log_factorial(n - l - 1)
                    - std::log(2.0 * n)
                    - log_factorial(n + l);
    double norm = std::sqrt(std::exp(log_norm));

    double L = assoc_laguerre(k, alpha, rho);
    double R = norm * std::exp(-rho / 2.0) * std::pow(rho, l) * L;
    return R;
}

inline double radial_wavefunction_sq(double r, int n, int l, double a0 = BOHR_RADIUS_A0) {
    double R = radial_wavefunction(r, n, l, a0);
    return R * R;
}

// =============================================================================
// Spherical Harmonics Y_l^m(θ, φ)
// =============================================================================

/// Normalization constant for Spherical Harmonic Y_l^m
inline double spherical_harmonic_norm(int l, int m) {
    m = std::abs(m);
    double log_norm = std::log((2.0 * l + 1.0) / (4.0 * M_PI))
                    + log_factorial(l - m)
                    - log_factorial(l + m);
    return std::sqrt(std::exp(log_norm));
}

/// Complex Spherical Harmonic Y_l^m(θ, φ)
inline std::complex<double> spherical_harmonic_complex(int l, int m, double theta, double phi) {
    double x = std::cos(theta);
    int abs_m = std::abs(m);
    double Plm = assoc_legendre(l, abs_m, x);
    double N = spherical_harmonic_norm(l, abs_m);

    double phase_factor = (m >= 0) ? 1.0 : (std::pow(-1.0, abs_m));
    std::complex<double> exp_imphi(std::cos(m * phi), std::sin(m * phi));
    return phase_factor * N * Plm * exp_imphi;
}

/// Real Spherical Harmonic Y_{lm}(θ, φ) for chemistry representation:
///   m = 0 : Y_l^0
///   m > 0 : sqrt(2) * (-1)^m * Re(Y_l^m) = sqrt(2) * N * P_l^m * cos(m*φ)
///   m < 0 : sqrt(2) * (-1)^|m| * Im(Y_l^{|m|}) = sqrt(2) * N * P_l^{|m|} * sin(|m|*φ)
inline double spherical_harmonic_real(int l, int m, double theta, double phi) {
    double x = std::cos(theta);
    int abs_m = std::abs(m);
    double Plm = assoc_legendre(l, abs_m, x);
    double N = spherical_harmonic_norm(l, abs_m);

    if (m == 0) {
        return N * Plm;
    } else if (m > 0) {
        return std::sqrt(2.0) * N * Plm * std::cos(m * phi);
    } else {
        return std::sqrt(2.0) * N * Plm * std::sin(abs_m * phi);
    }
}

// =============================================================================
// Full Hydrogen Wavefunction & Probability Density
// =============================================================================

/// Evaluate real wavefunction value ψ(r, θ, φ) at a spherical point
inline double wavefunction_real(double r, double theta, double phi, int n, int l, int m,
                                double a0 = BOHR_RADIUS_A0) {
    double R = radial_wavefunction(r, n, l, a0);
    double Y = spherical_harmonic_real(l, m, theta, phi);
    return R * Y;
}

/// Evaluate complex wavefunction ψ(r, θ, φ) at a spherical point
inline std::complex<double> wavefunction_complex(double r, double theta, double phi,
                                                int n, int l, int m,
                                                double a0 = BOHR_RADIUS_A0) {
    double R = radial_wavefunction(r, n, l, a0);
    std::complex<double> Y = spherical_harmonic_complex(l, m, theta, phi);
    return R * Y;
}

/// Spatial probability density |ψ(r, θ, φ)|² in Cartesian space (per unit volume)
inline double probability_density(double r, double theta, double phi, int n, int l, int m,
                                  bool use_real_orbitals = true, double a0 = BOHR_RADIUS_A0) {
    if (use_real_orbitals) {
        double psi = wavefunction_real(r, theta, phi, n, l, m, a0);
        return psi * psi;
    } else {
        auto psi = wavefunction_complex(r, theta, phi, n, l, m, a0);
        return std::norm(psi);
    }
}

// =============================================================================
// Coordinate Transformations
// =============================================================================

/// Convert spherical (r, theta, phi) -> Cartesian (x, y, z) with Y-up
inline Vec3 spherical_to_cartesian(float r, float theta, float phi) {
    return {
        r * std::sin(theta) * std::cos(phi),
        r * std::cos(theta),
        r * std::sin(theta) * std::sin(phi)
    };
}

/// Convert Cartesian (x, y, z) with Y-up -> spherical (r, theta, phi)
inline void cartesian_to_spherical(float x, float y, float z,
                                   float& r, float& theta, float& phi) {
    r = std::sqrt(x * x + y * y + z * z);
    if (r < 1e-8f) {
        theta = 0.0f;
        phi = 0.0f;
        return;
    }
    theta = std::acos(std::clamp(y / r, -1.0f, 1.0f));
    phi = std::atan2(z, x);
    if (phi < 0.0f) phi += static_cast<float>(2.0 * M_PI);
}

// =============================================================================
// Probability Current (Quantum Flow for Complex Eigenstates)
// =============================================================================
//
// Probability current: j = (hbar / m_e) * Im(ψ* ∇ψ)
// For complex eigenstate with quantum number m:
//   v_phi = hbar * m / (m_e * r * sin(theta))
// This yields a real circulating probability current around the Y (polar) axis!
//
inline Vec3 probability_flow(float px, float py, float pz, int m_quantum) {
    if (m_quantum == 0) return {0.0f, 0.0f, 0.0f};

    float r, theta, phi;
    cartesian_to_spherical(px, py, pz, r, theta, phi);
    if (r < 1e-4f) return {0.0f, 0.0f, 0.0f};

    float sin_theta = std::sin(theta);
    if (std::abs(sin_theta) < 1e-4f) sin_theta = (sin_theta >= 0.0f ? 1e-4f : -1e-4f);

    float v_mag = static_cast<float>(m_quantum) / (r * sin_theta);
    v_mag = std::clamp(v_mag, -10.0f, 10.0f); // Numerical safety clamp

    // Tangent vector in direction of increasing phi (horizontal circle around Y):
    // r * sin(theta) * cos(phi) = x, r * sin(theta) * sin(phi) = z
    // d/dphi (x) = -z, d/dphi (z) = x
    return {
        -v_mag * std::sin(phi),
        0.0f,
        v_mag * std::cos(phi)
    };
}

// =============================================================================
// CDF-Inversion Samplers for Realtime Electron Cloud
// =============================================================================

/// Fast CDF inversion sampler for Radial coordinate: P(r) dr = r² |R_{nl}(r)|² dr
struct RadialSampler {
    int cached_n = -1;
    int cached_l = -1;
    std::vector<double> cdf;
    double r_max = 0.0;
    double dr = 0.0;
    static constexpr int NUM_BINS = 4096;

    double sample(int n, int l, std::mt19937& gen, double a0 = BOHR_RADIUS_A0) {
        if (n != cached_n || l != cached_l) {
            rebuild(n, l, a0);
        }
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double u = dist(gen);
        auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
        int idx = static_cast<int>(std::distance(cdf.begin(), it));
        return idx * dr;
    }

    void rebuild(int n, int l, double a0) {
        cached_n = n;
        cached_l = l;
        // Radial wavefunction decays exponentially beyond ~ (n^2 + 5n) a0
        r_max = std::max(20.0, 3.5 * n * n * a0 + 10.0 * a0);
        dr = r_max / (NUM_BINS - 1);

        cdf.resize(NUM_BINS);
        double sum = 0.0;
        for (int i = 0; i < NUM_BINS; ++i) {
            double r = i * dr;
            double pdf = r * r * radial_wavefunction_sq(r, n, l, a0);
            sum += pdf;
            cdf[i] = sum;
        }
        if (sum > 0.0) {
            for (double& v : cdf) v /= sum;
        }
    }
};

/// Polar Angle CDF Sampler: P(θ) dθ = sin(θ) |P_l^m(cos θ)|² dθ
struct ThetaSampler {
    int cached_l = -1;
    int cached_m = -1;
    std::vector<double> cdf;
    double dtheta = 0.0;
    static constexpr int NUM_BINS = 2048;

    double sample(int l, int m, std::mt19937& gen) {
        m = std::abs(m);
        if (l != cached_l || m != cached_m) {
            rebuild(l, m);
        }
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double u = dist(gen);
        auto it = std::lower_bound(cdf.begin(), cdf.end(), u);
        int idx = static_cast<int>(std::distance(cdf.begin(), it));
        return idx * dtheta;
    }

    void rebuild(int l, int m) {
        cached_l = l;
        cached_m = m;
        dtheta = M_PI / (NUM_BINS - 1);

        cdf.resize(NUM_BINS);
        double sum = 0.0;
        for (int i = 0; i < NUM_BINS; ++i) {
            double theta = i * dtheta;
            double Plm = assoc_legendre(l, m, std::cos(theta));
            double pdf = std::sin(theta) * Plm * Plm;
            sum += pdf;
            cdf[i] = sum;
        }
        if (sum > 0.0) {
            for (double& v : cdf) v /= sum;
        }
    }
};

/// Azimuthal angle CDF/rejection sampler for Real Orbitals:
/// For m = 0 or complex states, phi is uniform in [0, 2π).
/// For real orbitals with m > 0 (cos(m*phi)^2) or m < 0 (sin(|m|*phi)^2):
inline double sample_phi(int m, bool use_real_orbitals, std::mt19937& gen) {
    std::uniform_real_distribution<double> dist(0.0, 2.0 * M_PI);
    if (!use_real_orbitals || m == 0) {
        return dist(gen);
    }
    // Rejection sample from cos^2(m*phi) or sin^2(m*phi)
    std::uniform_real_distribution<double> dist_u(0.0, 1.0);
    int abs_m = std::abs(m);
    for (int iter = 0; iter < 100; ++iter) {
        double candidate = dist(gen);
        double p = (m > 0) ? std::cos(abs_m * candidate) : std::sin(abs_m * candidate);
        if (dist_u(gen) < p * p) {
            return candidate;
        }
    }
    return dist(gen);
}

// =============================================================================
// Quantum Energy Levels & Photon Transition Physics
// =============================================================================

/// Energy level of hydrogen atom in eV: E_n = -13.606 eV / n^2
inline double energy_level_ev(int n) {
    if (n < 1) return -RYDBERG_EV;
    return -RYDBERG_EV / (static_cast<double>(n) * n);
}

/// Energy difference between two states: ΔE = |E_n2 - E_n1| (eV)
inline double transition_energy_ev(int n1, int n2) {
    return std::abs(energy_level_ev(n2) - energy_level_ev(n1));
}

/// Transition photon wavelength in nanometers: λ = hc / ΔE
inline double photon_wavelength_nm(int n1, int n2) {
    double dE = transition_energy_ev(n1, n2);
    if (dE < 1e-6) return 0.0;
    return HC_EV_NM / dE;
}

/// Convert optical wavelength (nm) to approximate RGB color
inline Color4 wavelength_to_rgb(double wavelength_nm) {
    double r = 0.0, g = 0.0, b = 0.0;

    if (wavelength_nm < 380.0) {
        // Ultraviolet -> Deep luminous violet
        r = 0.6; g = 0.1; b = 0.9;
    } else if (wavelength_nm >= 380.0 && wavelength_nm < 440.0) {
        // Violet
        r = -(wavelength_nm - 440.0) / (440.0 - 380.0);
        g = 0.0;
        b = 1.0;
    } else if (wavelength_nm >= 440.0 && wavelength_nm < 490.0) {
        // Blue / Cyan (e.g. Balmer H-beta 486 nm)
        r = 0.0;
        g = (wavelength_nm - 440.0) / (490.0 - 440.0);
        b = 1.0;
    } else if (wavelength_nm >= 490.0 && wavelength_nm < 510.0) {
        // Cyan / Green
        r = 0.0;
        g = 1.0;
        b = -(wavelength_nm - 510.0) / (510.0 - 490.0);
    } else if (wavelength_nm >= 510.0 && wavelength_nm < 580.0) {
        // Green / Yellow
        r = (wavelength_nm - 510.0) / (580.0 - 510.0);
        g = 1.0;
        b = 0.0;
    } else if (wavelength_nm >= 580.0 && wavelength_nm < 645.0) {
        // Yellow / Orange
        r = 1.0;
        g = -(wavelength_nm - 645.0) / (645.0 - 580.0);
        b = 0.0;
    } else if (wavelength_nm >= 645.0 && wavelength_nm <= 750.0) {
        // Red (e.g. Balmer H-alpha 656 nm)
        r = 1.0;
        g = 0.0;
        b = 0.0;
    } else {
        // Infrared -> Deep crimson/ruby
        r = 0.8; g = 0.05; b = 0.1;
    }

    // Intensity falloff at the visual limits
    double factor = 1.0;
    if (wavelength_nm >= 380.0 && wavelength_nm < 420.0) {
        factor = 0.3 + 0.7 * (wavelength_nm - 380.0) / (420.0 - 380.0);
    } else if (wavelength_nm >= 700.0 && wavelength_nm <= 750.0) {
        factor = 0.3 + 0.7 * (750.0 - wavelength_nm) / (750.0 - 700.0);
    }

    return {
        static_cast<float>(std::clamp(r * factor, 0.0, 1.0)),
        static_cast<float>(std::clamp(g * factor, 0.0, 1.0)),
        static_cast<float>(std::clamp(b * factor, 0.0, 1.0)),
        1.0f
    };
}

/// Get spectroscopic notation name for quantum numbers (e.g., "1s", "2p_z", "3d_xy")
inline std::string orbital_name(int n, int l, int m, bool real_rep = true) {
    const char subshell_letters[] = {'s', 'p', 'd', 'f', 'g', 'h'};
    char subshell = (l >= 0 && l < 6) ? subshell_letters[l] : '?';
    std::string base = std::to_string(n) + subshell;

    if (!real_rep) {
        return base + " (m=" + std::to_string(m) + ")";
    }

    if (l == 0) return base;
    if (l == 1) {
        if (m == 0) return base + "z";
        if (m == 1) return base + "x";
        if (m == -1) return base + "y";
    }
    if (l == 2) {
        if (m == 0) return base + "z²";
        if (m == 1) return base + "xz";
        if (m == -1) return base + "yz";
        if (m == 2) return base + "(x²-y²)";
        if (m == -2) return base + "xy";
    }
    return base + " (m=" + std::to_string(m) + ")";
}

} // namespace orbital

#endif // ORBITAL_MATH_H
