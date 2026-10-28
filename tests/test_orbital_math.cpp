// =============================================================================
// test_orbital_math.cpp — Comprehensive Unit Tests for Quantum Math
// =============================================================================

#include "../include/orbital_math.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <string>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(); \
    struct Register_##name { \
        Register_##name() { \
            std::cout << "[TEST] " #name "... "; \
            try { \
                test_##name(); \
                std::cout << "PASSED\n"; \
                tests_passed++; \
            } catch (const std::exception& e) { \
                std::cout << "FAILED: " << e.what() << "\n"; \
                tests_failed++; \
            } \
        } \
    } register_##name; \
    static void test_##name()

#define ASSERT_NEAR(val, expected, tol) \
    do { \
        double _v = (val), _e = (expected), _t = (tol); \
        if (std::abs(_v - _e) > _t) { \
            throw std::runtime_error( \
                "Expected " + std::to_string(_e) + " ± " + std::to_string(_t) + \
                ", got " + std::to_string(_v)); \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            throw std::runtime_error("Assertion failed: " #cond); \
        } \
    } while(0)

// -----------------------------------------------------------------------------
// Test 1: Associated Laguerre Polynomials
// Known values:
//   L_0^\alpha(x) = 1
//   L_1^1(x) = 2 - x
//   L_2^0(x) = (x^2 - 4x + 2) / 2
// -----------------------------------------------------------------------------
TEST(assoc_laguerre) {
    ASSERT_NEAR(orbital::assoc_laguerre(0, 0, 3.5), 1.0, 1e-10);
    ASSERT_NEAR(orbital::assoc_laguerre(0, 5, 2.0), 1.0, 1e-10);
    ASSERT_NEAR(orbital::assoc_laguerre(1, 1, 0.5), 2.0 - 0.5, 1e-10);
    ASSERT_NEAR(orbital::assoc_laguerre(1, 3, 2.0), 1.0 + 3.0 - 2.0, 1e-10);

    // L_2^0(x) at x = 2: (4 - 8 + 2)/2 = -1
    ASSERT_NEAR(orbital::assoc_laguerre(2, 0, 2.0), -1.0, 1e-10);
}

// -----------------------------------------------------------------------------
// Test 2: Associated Legendre Polynomials
// Known values:
//   P_0^0(x) = 1
//   P_1^0(x) = x
//   P_1^1(x) = -sqrt(1 - x^2)  (with Condon-Shortley phase)
//   P_2^0(x) = (3x^2 - 1)/2
//   P_2^1(x) = -3x sqrt(1 - x^2)
//   P_2^2(x) = 3(1 - x^2)
// -----------------------------------------------------------------------------
TEST(assoc_legendre) {
    double x = 0.6;
    ASSERT_NEAR(orbital::assoc_legendre(0, 0, x), 1.0, 1e-10);
    ASSERT_NEAR(orbital::assoc_legendre(1, 0, x), x, 1e-10);
    ASSERT_NEAR(orbital::assoc_legendre(1, 1, x), -std::sqrt(1.0 - x*x), 1e-10);
    ASSERT_NEAR(orbital::assoc_legendre(2, 0, x), 0.5 * (3.0*x*x - 1.0), 1e-10);
    ASSERT_NEAR(orbital::assoc_legendre(2, 1, x), -3.0 * x * std::sqrt(1.0 - x*x), 1e-10);
    ASSERT_NEAR(orbital::assoc_legendre(2, 2, x), 3.0 * (1.0 - x*x), 1e-10);
}

// -----------------------------------------------------------------------------
// Test 3: Radial Wavefunction Normalization & Specific Values
// R_10(r) = 2 * exp(-r) in atomic units
// R_20(r) = (1/sqrt(2)) * (1 - r/2) * exp(-r/2) -> node at r = 2
// -----------------------------------------------------------------------------
TEST(radial_wavefunction) {
    // 1s at r = 0 should be 2.0
    ASSERT_NEAR(orbital::radial_wavefunction(0.0, 1, 0), 2.0, 1e-8);
    // 1s at r = 1.0: 2 * exp(-1) = 0.73575888
    ASSERT_NEAR(orbital::radial_wavefunction(1.0, 1, 0), 2.0 * std::exp(-1.0), 1e-8);

    // 2s radial node at r = 2.0: R_20(2.0) == 0
    ASSERT_NEAR(orbital::radial_wavefunction(2.0, 2, 0), 0.0, 1e-8);

    // Numerical integration of |R_nl(r)|^2 * r^2 from 0 to 50 should equal 1.0
    int steps = 10000;
    double dr = 50.0 / steps;
    double integral_1s = 0.0;
    double integral_2p = 0.0;
    for (int i = 0; i < steps; ++i) {
        double r = (i + 0.5) * dr;
        integral_1s += orbital::radial_wavefunction_sq(r, 1, 0) * r * r * dr;
        integral_2p += orbital::radial_wavefunction_sq(r, 2, 1) * r * r * dr;
    }
    ASSERT_NEAR(integral_1s, 1.0, 1e-3);
    ASSERT_NEAR(integral_2p, 1.0, 1e-3);
}

// -----------------------------------------------------------------------------
// Test 4: Energy Levels & Photon Transition Wavelengths
// Ground state: -13.6057 eV
// 3 -> 2 (Balmer H-alpha): ~656.3 nm (Red)
// 4 -> 2 (Balmer H-beta):  ~486.1 nm (Cyan)
// 2 -> 1 (Lyman-alpha):   ~121.6 nm (UV)
// -----------------------------------------------------------------------------
TEST(quantum_transitions) {
    ASSERT_NEAR(orbital::energy_level_ev(1), -13.60569, 1e-3);
    ASSERT_NEAR(orbital::energy_level_ev(2), -3.40142, 1e-3);

    double lambda_32 = orbital::photon_wavelength_nm(3, 2);
    ASSERT_NEAR(lambda_32, 656.3, 1.0); // Balmer alpha ~ 656.3 nm

    double lambda_42 = orbital::photon_wavelength_nm(4, 2);
    ASSERT_NEAR(lambda_42, 486.1, 1.0); // Balmer beta ~ 486.1 nm

    double lambda_21 = orbital::photon_wavelength_nm(2, 1);
    ASSERT_NEAR(lambda_21, 121.6, 1.0); // Lyman alpha ~ 121.6 nm

    // Test RGB color synthesis for 656 nm (Red)
    auto col_red = orbital::wavelength_to_rgb(656.3);
    ASSERT_TRUE(col_red.r > 0.8f && col_red.g < 0.2f && col_red.b < 0.2f);

    // Test RGB color synthesis for 486 nm (Cyan)
    auto col_cyan = orbital::wavelength_to_rgb(486.1);
    ASSERT_TRUE(col_cyan.b > 0.8f && col_cyan.g > 0.5f);
}

// -----------------------------------------------------------------------------
// Test 5: Probability Sampler Convergence
// Verify mean radius for 1s: <r> = (3/2) a0 = 1.5
// -----------------------------------------------------------------------------
TEST(radial_sampler_expectation) {
    orbital::RadialSampler sampler;
    std::mt19937 gen(42);
    double sum_r = 0.0;
    const int N = 20000;
    for (int i = 0; i < N; ++i) {
        sum_r += sampler.sample(1, 0, gen);
    }
    double mean_r = sum_r / N;
    // Expected <r> for 1s is 1.5 a0
    ASSERT_NEAR(mean_r, 1.5, 0.05);
}

int main() {
    std::cout << "\n============================================\n";
    std::cout << "  Hydrogen Quantum Math Validation Tests\n";
    std::cout << "============================================\n";

    std::cout << "\nResults: " << tests_passed << " passed, "
              << tests_failed << " failed.\n";
    return tests_failed == 0 ? 0 : 1;
}
