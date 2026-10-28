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
    // Local pos is (x, z) on horizontal plane Y=0
    // To see vertical orientation (like pz), let's map: local x -> X, local y -> Y (vertical)
    vec3 p = vec3(vLocalPos.x, vLocalPos.y, 0.0);
    float r = length(p);

    if (r < 1e-5) {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    float theta = acos(clamp(p.y / r, -1.0, 1.0));
    float phi = atan(p.z, p.x);
    if (phi < 0.0) phi += 2.0 * PI;

    float rho = 2.0 * r / float(uN);
    int k = uN - uL - 1;
    int alpha = 2 * uL + 1;
    float L = assocLaguerre(k, alpha, rho);

    float normR = pow(2.0 / float(uN), 1.5)
                * sqrt(factorial(uN - uL - 1) / (2.0 * float(uN) * factorial(uN + uL)));
    float R = normR * exp(-rho * 0.5) * pow(rho, float(uL)) * L;

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
    float prob = psi * psi;

    // Base color by quantum phase:
    // Positive phase = Warm Amber/Red, Negative phase = Cool Electric Cyan
    vec3 baseColor = vec3(0.0);
    float intensity = clamp(prob * pow(5.0, float(uN)) * uBrightness, 0.0, 1.0);

    if (psi >= 0.0) {
        baseColor = mix(vec3(0.05, 0.0, 0.0), vec3(1.0, 0.35, 0.05), intensity);
    } else {
        baseColor = mix(vec3(0.0, 0.02, 0.08), vec3(0.05, 0.70, 1.0), intensity);
    }

    // Nodal line enhancement (where psi == 0)
    if (uShowNodalLines) {
        float grad = fwidth(psi);
        float nodeLine = 1.0 - smoothstep(0.0, max(grad * 1.5, 0.0005), abs(psi));
        baseColor = mix(baseColor, vec3(1.0, 1.0, 1.0), nodeLine * 0.85);
    }

    // Iso-probability contour rings
    if (uShowContours) {
        float contourFreq = 15.0;
        float cVal = fract(sqrt(prob) * contourFreq * 10.0);
        float line = smoothstep(0.05, 0.0, abs(cVal - 0.5));
        baseColor += vec3(0.25) * line;
    }

    float alpha = clamp(intensity * 1.5 + 0.1, 0.1, 0.95);
    FragColor = vec4(baseColor, alpha);
}
