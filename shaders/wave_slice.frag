#version 330 core

in vec2 vLocalPos;
out vec4 FragColor;

uniform int uN;
uniform int uL;
uniform int uM;
uniform bool uRealOrbitals;
uniform bool uShowContours;
uniform bool uShowNodalLines;
uniform float uBrightness;
uniform float uPsiMax;
uniform int uSlicePlane; // 0 = XY (meridional), 1 = YZ (sagittal), 2 = XZ (equatorial)

const float PI = 3.14159265358979323846;

// Special Polynomials in GLSL
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

void main() {
    // Map local 2D coordinates (u, v) to 3D space according to slice plane
    vec3 p;
    if (uSlicePlane == 1) {
        // YZ sagittal plane
        p = vec3(0.0, vLocalPos.y, vLocalPos.x);
    } else if (uSlicePlane == 2) {
        // XZ equatorial plane
        p = vec3(vLocalPos.x, 0.0, vLocalPos.y);
    } else {
        // Default XY meridional plane (vertical)
        p = vec3(vLocalPos.x, vLocalPos.y, 0.0);
    }

    float r = length(p);

    // Deep space plate background
    vec3 baseColor = vec3(0.04, 0.05, 0.08);

    // Grid lines every 5 a0 and coordinate axes
    float gridStep = 5.0;
    vec2 gridDist = abs(fract(vLocalPos / gridStep + 0.5) - 0.5) * gridStep;
    float gridLine = smoothstep(0.15, 0.0, min(gridDist.x, gridDist.y));
    baseColor += vec3(0.05, 0.07, 0.10) * gridLine;

    // Coordinate crosshair axes
    float axisLine = smoothstep(0.20, 0.0, min(abs(vLocalPos.x), abs(vLocalPos.y)));
    baseColor += vec3(0.12, 0.16, 0.25) * axisLine;

    // Radial distance circles at 5, 10, 15, 20 a0
    float rMod = abs(fract(r / 5.0 + 0.5) - 0.5) * 5.0;
    float rCircle = smoothstep(0.14, 0.0, rMod);
    baseColor += vec3(0.06, 0.10, 0.15) * rCircle;

    if (r < 1e-4) {
        // Central nucleus marker
        FragColor = vec4(1.0, 0.95, 0.85, 1.0);
        return;
    }

    // Spherical coordinates
    float theta = acos(clamp(p.y / r, -1.0, 1.0));
    float phi = atan(p.z, p.x);
    if (phi < 0.0) phi += 2.0 * PI;

    // Radial wavefunction R_nl(r)
    float rho = 2.0 * r / float(uN);
    int k = uN - uL - 1;
    int alpha = 2 * uL + 1;
    float L = assocLaguerre(k, alpha, rho);

    float normR = pow(2.0 / float(uN), 1.5)
                * sqrt(factorial(uN - uL - 1) / (2.0 * float(uN) * factorial(uN + uL)));
    float R = normR * exp(-rho * 0.5) * pow(rho, float(uL)) * L;

    // Angular wavefunction Y_l^m
    int abs_m = abs(uM);
    float Plm = assocLegendre(uL, abs_m, cos(theta));
    float normY = sqrt((float(2 * uL + 1) / (4.0 * PI))
                     * (factorial(uL - abs_m) / factorial(uL + abs_m)));

    float Y = 0.0;
    if (uRealOrbitals) {
        if (uM == 0) Y = normY * Plm;
        else if (uM > 0) Y = sqrt(2.0) * normY * Plm * cos(float(uM) * phi);
        else Y = sqrt(2.0) * normY * Plm * sin(float(abs_m) * phi);
    } else {
        Y = normY * Plm;
    }

    float psi = R * Y;

    // Normalized wavefunction magnitude
    float refScale = max(uPsiMax, 1e-5);
    float normPsi = clamp(psi / refScale, -2.0, 2.0);
    float magnitude = abs(normPsi);
    float val = clamp(pow(magnitude, 0.45) * uBrightness, 0.0, 1.0);

    // Glowing quantum phase coloration:
    // Positive phase (+): warm glowing Amber / Gold / Orange
    // Negative phase (-): cool glowing Electric Cyan / Azure
    vec3 posColor = mix(vec3(0.18, 0.05, 0.02), vec3(1.0, 0.52, 0.10), val);
    posColor = mix(posColor, vec3(1.0, 0.95, 0.70), pow(val, 2.5));

    vec3 negColor = mix(vec3(0.02, 0.06, 0.18), vec3(0.05, 0.75, 1.0), val);
    negColor = mix(negColor, vec3(0.80, 0.98, 1.00), pow(val, 2.5));

    if (normPsi >= 0.0) {
        baseColor = mix(baseColor, posColor, val);
    } else {
        baseColor = mix(baseColor, negColor, val);
    }

    // Nodal line enhancement (where psi == 0)
    if (uShowNodalLines) {
        float nodeWidth = 0.045;
        float nodeLine = smoothstep(nodeWidth, 0.0, magnitude);
        float extentLimit = float(uN * uN) * 2.5 + 8.0;
        float regionMask = smoothstep(extentLimit, 0.0, r);
        baseColor = mix(baseColor, vec3(1.0, 0.95, 0.40), nodeLine * regionMask * 0.90);
    }

    // Iso-probability contours
    if (uShowContours) {
        float contour = smoothstep(0.08, 0.0, abs(fract(val * 7.0) - 0.5) * 0.5);
        baseColor += vec3(0.30, 0.35, 0.45) * contour * val;
    }

    FragColor = vec4(baseColor, 0.98);
}
