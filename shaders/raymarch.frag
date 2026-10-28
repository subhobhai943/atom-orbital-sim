#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform vec3 uCameraPos;
uniform mat4 uInvViewProj;
uniform int uN;
uniform int uL;
uniform int uM;
uniform bool uRealOrbitals;
uniform int uPalette; // 0=Fire, 1=Cyan, 2=Phase, 3=Plasma
uniform float uDensity;
uniform float uBoundRadius;
uniform int uMaxSteps;
uniform bool uEnableClip;
uniform vec4 uClipPlane;

const float PI = 3.14159265358979323846;

// -----------------------------------------------------------------------------
// Special Polynomials in GLSL
// -----------------------------------------------------------------------------
float assocLaguerre(int k, int alpha, float x) {
    if (k <= 0) return 1.0;
    float Lm2 = 1.0;
    float Lm1 = 1.0 + float(alpha) - x;
    if (k == 1) return Lm1;

    float L = Lm1;
    for (int j = 2; j <= 6; ++j) {
        if (j > k) break;
        L = ((float(2 * j - 1 + alpha) - x) * Lm1 - float(j - 1 + alpha) * Lm2) / float(j);
        Lm2 = Lm1;
        Lm1 = L;
    }
    return L;
}

float assocLegendre(int l, int m, float x) {
    int abs_m = abs(m);
    if (abs_m > l) return 0.0;
    x = clamp(x, -1.0, 1.0);

    float Pmm = 1.0;
    if (abs_m > 0) {
        float somx2 = sqrt(max(0.0, (1.0 - x) * (1.0 + x)));
        float fact = 1.0;
        for (int i = 1; i <= 6; ++i) {
            if (i > abs_m) break;
            Pmm *= -fact * somx2;
            fact += 2.0;
        }
    }
    if (l == abs_m) return Pmm;

    float Pm1m = x * float(2 * abs_m + 1) * Pmm;
    if (l == abs_m + 1) return Pm1m;

    float Plm = Pm1m;
    for (int ll = abs_m + 2; ll <= 6; ++ll) {
        if (ll > l) break;
        Plm = ((float(2 * ll - 1) * x * Pm1m - float(ll + abs_m - 1) * Pmm)) / float(ll - abs_m);
        Pmm = Pm1m;
        Pm1m = Plm;
    }
    return Plm;
}

float factorial(int num) {
    if (num <= 1) return 1.0;
    float res = 1.0;
    for (int i = 2; i <= 10; ++i) {
        if (i > num) break;
        res *= float(i);
    }
    return res;
}

// -----------------------------------------------------------------------------
// Wavefunction Evaluation
// -----------------------------------------------------------------------------
float evaluateWavefunction(vec3 p) {
    float r = length(p);
    if (r < 1e-4 || r > uBoundRadius) return 0.0;

    float theta = acos(clamp(p.y / r, -1.0, 1.0));
    float phi = atan(p.z, p.x);
    if (phi < 0.0) phi += 2.0 * PI;

    // Radial part
    float rho = 2.0 * r / float(uN);
    int k = uN - uL - 1;
    int alpha = 2 * uL + 1;
    float L = assocLaguerre(k, alpha, rho);

    float normR = pow(2.0 / float(uN), 1.5)
                * sqrt(factorial(uN - uL - 1) / (2.0 * float(uN) * factorial(uN + uL)));
    float R = normR * exp(-rho * 0.5) * pow(rho, float(uL)) * L;

    // Angular part
    int abs_m = abs(uM);
    float Plm = assocLegendre(uL, abs_m, cos(theta));
    float normY = sqrt((float(2 * uL + 1) / (4.0 * PI))
                     * (factorial(uL - abs_m) / factorial(uL + abs_m)));

    float Y = 0.0;
    if (uRealOrbitals) {
        if (uM == 0) {
            Y = normY * Plm;
        } else if (uM > 0) {
            Y = sqrt(2.0) * normY * Plm * cos(float(uM) * phi);
        } else {
            Y = sqrt(2.0) * normY * Plm * sin(float(abs_m) * phi);
        }
    } else {
        // Complex probability density is independent of phi
        Y = normY * Plm;
    }

    return R * Y;
}

// -----------------------------------------------------------------------------
// Color Palettes
// -----------------------------------------------------------------------------
vec3 sampleColor(float psi, float density, int palette) {
    float val = clamp(density * 15.0, 0.0, 1.0);

    if (palette == 0) {
        // Thermal Fire
        vec3 c0 = vec3(0.02, 0.01, 0.05);
        vec3 c1 = vec3(0.35, 0.02, 0.60);
        vec3 c2 = vec3(0.85, 0.05, 0.05);
        vec3 c3 = vec3(1.00, 0.55, 0.00);
        vec3 c4 = vec3(1.00, 0.95, 0.30);
        vec3 c5 = vec3(1.00, 1.00, 1.00);

        if (val < 0.2) return mix(c0, c1, val / 0.2);
        if (val < 0.4) return mix(c1, c2, (val - 0.2) / 0.2);
        if (val < 0.6) return mix(c2, c3, (val - 0.4) / 0.2);
        if (val < 0.8) return mix(c3, c4, (val - 0.6) / 0.2);
        return mix(c4, c5, (val - 0.8) / 0.2);
    } else if (palette == 1) {
        // Neon Cyan
        vec3 c0 = vec3(0.01, 0.03, 0.12);
        vec3 c1 = vec3(0.05, 0.25, 0.70);
        vec3 c2 = vec3(0.00, 0.80, 0.95);
        vec3 c3 = vec3(0.60, 0.98, 1.00);
        return mix(mix(c0, c1, clamp(val * 2.0, 0.0, 1.0)),
                   mix(c2, c3, clamp((val - 0.5) * 2.0, 0.0, 1.0)),
                   step(0.5, val));
    } else if (palette == 2) {
        // Quantum Phase: Red for +, Blue for -
        float normPsi = clamp(psi * 8.0, -1.0, 1.0);
        if (normPsi >= 0.0) {
            return mix(vec3(0.8, 0.1, 0.0), vec3(1.0, 0.8, 0.2), normPsi);
        } else {
            return mix(vec3(0.0, 0.2, 0.8), vec3(0.2, 0.8, 1.0), -normPsi);
        }
    } else {
        // Plasma
        vec3 c0 = vec3(0.05, 0.01, 0.15);
        vec3 c1 = vec3(0.60, 0.05, 0.65);
        vec3 c2 = vec3(0.95, 0.30, 0.35);
        vec3 c3 = vec3(1.00, 0.95, 0.50);
        if (val < 0.33) return mix(c0, c1, val / 0.33);
        if (val < 0.66) return mix(c1, c2, (val - 0.33) / 0.33);
        return mix(c2, c3, (val - 0.66) / 0.34);
    }
}

// Ray-Sphere intersection
bool intersectSphere(vec3 ro, vec3 rd, float radius, out float tNear, out float tFar) {
    float b = dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float disc = b * b - c;
    if (disc < 0.0) return false;
    float s = sqrt(disc);
    tNear = -b - s;
    tFar  = -b + s;
    if (tFar < 0.0) return false;
    tNear = max(tNear, 0.0);
    return true;
}

void main() {
    // Reconstruct world ray from NDC
    vec4 clipNear = vec4(vUV * 2.0 - 1.0, -1.0, 1.0);
    vec4 clipFar  = vec4(vUV * 2.0 - 1.0,  1.0, 1.0);
    vec4 worldNear = uInvViewProj * clipNear;
    vec4 worldFar  = uInvViewProj * clipFar;
    worldNear /= worldNear.w;
    worldFar  /= worldFar.w;

    vec3 ro = uCameraPos;
    vec3 rd = normalize(worldFar.xyz - worldNear.xyz);

    float tNear, tFar;
    if (!intersectSphere(ro, rd, uBoundRadius, tNear, tFar)) {
        discard;
    }

    float tStep = (tFar - tNear) / float(uMaxSteps);
    float t = tNear + 0.5 * tStep;

    vec3 accumColor = vec3(0.0);
    float transmittance = 1.0;

    for (int i = 0; i < 128; ++i) {
        if (i >= uMaxSteps || t > tFar || transmittance < 0.02) break;

        vec3 p = ro + rd * t;

        // Clipping plane check
        bool clipped = false;
        if (uEnableClip) {
            float dist = dot(p, uClipPlane.xyz) + uClipPlane.w;
            if (dist < 0.0) clipped = true;
        }

        if (!clipped) {
            float psi = evaluateWavefunction(p);
            float probDensity = psi * psi;

            if (probDensity > 1e-6) {
                float sampleDensity = probDensity * uDensity;
                vec3 col = sampleColor(psi, probDensity, uPalette);

                float stepExtinction = exp(-sampleDensity * tStep * 100.0);
                float stepAbsorption = (1.0 - stepExtinction);

                accumColor += col * stepAbsorption * transmittance;
                transmittance *= stepExtinction;
            }
        }

        t += tStep;
    }

    float alpha = 1.0 - transmittance;
    FragColor = vec4(accumColor, alpha);
}
