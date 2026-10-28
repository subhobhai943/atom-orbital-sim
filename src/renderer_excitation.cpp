#include "../include/renderer_excitation.h"
#include <iostream>
#include <cmath>

namespace orbital {

ExcitationRenderer::ExcitationRenderer() = default;

ExcitationRenderer::~ExcitationRenderer() {
    if (m_particleVao) glDeleteVertexArrays(1, &m_particleVao);
    if (m_particleVbo) glDeleteBuffers(1, &m_particleVbo);
    if (m_photonVao) glDeleteVertexArrays(1, &m_photonVao);
    if (m_photonVbo) glDeleteBuffers(1, &m_photonVbo);
}

std::string ExcitationRenderer::getSpectralSeriesName(int n_lower) const {
    switch (n_lower) {
        case 1: return "Lyman Series (Ultraviolet)";
        case 2: return "Balmer Series (Visible Spectrum)";
        case 3: return "Paschen Series (Infrared)";
        case 4: return "Brackett Series (Far Infrared)";
        case 5: return "Pfund Series (Far Infrared)";
        default: return "Humphreys / Higher Series";
    }
}

bool ExcitationRenderer::init() {
    m_particleShader = std::make_unique<Shader>();
    if (!m_particleShader->loadFromFiles("shaders/particle.vert", "shaders/particle.frag")) {
        const std::string vertSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uBaseSize;
out vec4 vColor;
void main() {
    vec4 viewPos = uView * uModel * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;
    float depth = max(-viewPos.z, 0.1);
    gl_PointSize = clamp(uBaseSize * (600.0 / depth), 1.5, 64.0);
    vColor = aColor;
}
)";
        const std::string fragSrc = R"(#version 330 core
in vec4 vColor;
uniform float uBrightness;
out vec4 FragColor;
void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float distSq = dot(coord, coord) * 4.0;
    if (distSq > 1.0) discard;
    float falloff = exp(-3.2 * distSq) * (1.0 - distSq);
    float alpha = clamp(vColor.a * falloff * uBrightness, 0.0, 1.0);
    vec3 rgb = vColor.rgb * (1.0 + 0.3 * exp(-6.0 * distSq));
    FragColor = vec4(rgb, alpha);
}
)";
        m_particleShader->loadFromStrings(vertSrc, fragSrc);
    }

    m_photonShader = std::make_unique<Shader>();
    if (!m_photonShader->loadFromFiles("shaders/photon.vert", "shaders/photon.frag")) {
        const std::string vertSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)";
        const std::string fragSrc = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";
        m_photonShader->loadFromStrings(vertSrc, fragSrc);
    }

    glGenVertexArrays(1, &m_particleVao);
    glGenBuffers(1, &m_particleVbo);

    glBindVertexArray(m_particleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
    glBufferData(GL_ARRAY_BUFFER, 80000 * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    // Photon wavepacket buffer
    glGenVertexArrays(1, &m_photonVao);
    glGenBuffers(1, &m_photonVbo);

    return true;
}

void ExcitationRenderer::ensureInitialized(SimulationSettings& settings) {
    if (m_particles.empty()) {
        settings.exc_n1 = 1;
        settings.exc_l1 = 0;
        settings.exc_m1 = 0;
        settings.exc_n2 = 2;
        settings.exc_l2 = 1;
        settings.exc_m2 = 0;
        settings.photon_wavelength = photon_wavelength_nm(settings.exc_n1, settings.exc_n2);
        settings.photon_color = wavelength_to_rgb(settings.photon_wavelength);
        rebuildSuperpositionParticles(settings);
    }
}

void ExcitationRenderer::triggerTransition(SimulationSettings& settings, int target_n, int target_l, int target_m) {
    settings.exc_n1 = settings.n;
    settings.exc_l1 = settings.l;
    settings.exc_m1 = settings.m;

    settings.exc_n2 = target_n;
    settings.exc_l2 = target_l;
    settings.exc_m2 = target_m;

    settings.photon_wavelength = photon_wavelength_nm(settings.exc_n1, settings.exc_n2);
    settings.photon_color = wavelength_to_rgb(settings.photon_wavelength);

    settings.transition_progress = 0.0f;
    settings.transition_animating = true;

    if (settings.exc_n2 > settings.exc_n1) {
        // Absorption: Photon incoming from left (-X) towards atom
        settings.photon_incoming = true;
        settings.photon_outgoing = false;
        m_photonX = -m_photonTravelDistance;
    } else {
        // Emission: Photon emitted outgoing to right (+X)
        settings.photon_incoming = false;
        settings.photon_outgoing = true;
        m_photonX = 0.0f;
    }

    rebuildSuperpositionParticles(settings);
}

void ExcitationRenderer::rebuildSuperpositionParticles(SimulationSettings& settings) {
    const int count = 50000;
    m_particles.clear();
    m_particles.reserve(count);
    m_particleBuffer.resize(count * 7);

    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * float(M_PI));

    for (int i = 0; i < count; ++i) {
        // Sample from state 1
        float r1 = static_cast<float>(settings.radial_sampler.sample(settings.exc_n1, settings.exc_l1, settings.rng));
        float t1 = static_cast<float>(settings.theta_sampler.sample(settings.exc_l1, settings.exc_m1, settings.rng));
        float p1 = static_cast<float>(sample_phi(settings.exc_m1, settings.use_real_orbitals, settings.rng));
        Vec3 v1 = spherical_to_cartesian(r1, t1, p1);

        // Sample from state 2
        float r2 = static_cast<float>(settings.radial_sampler.sample(settings.exc_n2, settings.exc_l2, settings.rng));
        float t2 = static_cast<float>(settings.theta_sampler.sample(settings.exc_l2, settings.exc_m2, settings.rng));
        float p2 = static_cast<float>(sample_phi(settings.exc_m2, settings.use_real_orbitals, settings.rng));
        Vec3 v2 = spherical_to_cartesian(r2, t2, p2);

        Color4 c1 = map_orbital_color(0.7f, settings.palette);
        Color4 c2 = map_orbital_color(0.9f, settings.palette);

        ExcitationParticle ep;
        ep.pos1 = glm::vec3(v1.x, v1.y, v1.z);
        ep.pos2 = glm::vec3(v2.x, v2.y, v2.z);
        ep.current_pos = ep.pos1;
        ep.color1 = glm::vec4(c1.r, c1.g, c1.b, c1.a);
        ep.color2 = glm::vec4(c2.r, c2.g, c2.b, c2.a);
        ep.current_color = ep.color1;
        ep.phase_offset = phaseDist(settings.rng);

        m_particles.push_back(ep);

        size_t idx = i * 7;
        m_particleBuffer[idx + 0] = ep.pos1.x;
        m_particleBuffer[idx + 1] = ep.pos1.y;
        m_particleBuffer[idx + 2] = ep.pos1.z;
        m_particleBuffer[idx + 3] = ep.color1.r;
        m_particleBuffer[idx + 4] = ep.color1.g;
        m_particleBuffer[idx + 5] = ep.color1.b;
        m_particleBuffer[idx + 6] = ep.color1.a;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_particleBuffer.size() * sizeof(float), m_particleBuffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ExcitationRenderer::update(float deltaTime, SimulationSettings& settings) {
    m_animTime += deltaTime;

    if (m_particles.empty()) {
        ensureInitialized(settings);
    }

    if (!settings.transition_animating) {
        return;
    }

    float speed = settings.transition_speed * 1.2f;

    if (settings.photon_incoming) {
        // Photon travels from -50 to 0
        m_photonX += deltaTime * 28.0f * speed;
        if (m_photonX >= 0.0f) {
            m_photonX = 0.0f;
            settings.photon_incoming = false;
        }
    } else {
        // Transition progresses
        settings.transition_progress += deltaTime * speed;

        if (settings.photon_outgoing) {
            m_photonX += deltaTime * 28.0f * speed;
            if (m_photonX > m_photonTravelDistance) {
                settings.photon_outgoing = false;
            }
        }

        if (settings.transition_progress >= 1.0f) {
            settings.transition_progress = 1.0f;
            settings.transition_animating = false;
            settings.photon_outgoing = false;

            // State settled into final orbital
            settings.setOrbital(settings.exc_n2, settings.exc_l2, settings.exc_m2);
        }
    }

    // Update superposition particle cloud
    float t = settings.transition_progress;
    float smoothT = 0.5f - 0.5f * std::cos(t * float(M_PI));

    float deltaE = static_cast<float>(transition_energy_ev(settings.exc_n1, settings.exc_n2));
    float beatFreq = deltaE * 8.0f;

    for (size_t i = 0; i < m_particles.size(); ++i) {
        auto& p = m_particles[i];

        float dipoleOsc = 0.15f * std::sin(beatFreq * m_animTime + p.phase_offset) * std::sin(smoothT * float(M_PI));
        glm::vec3 base = glm::mix(p.pos1, p.pos2, smoothT);
        p.current_pos = base + glm::vec3(0.0f, dipoleOsc * 2.0f, 0.0f);
        p.current_color = glm::mix(p.color1, p.color2, smoothT);

        size_t idx = i * 7;
        m_particleBuffer[idx + 0] = p.current_pos.x;
        m_particleBuffer[idx + 1] = p.current_pos.y;
        m_particleBuffer[idx + 2] = p.current_pos.z;
        m_particleBuffer[idx + 3] = p.current_color.r;
        m_particleBuffer[idx + 4] = p.current_color.g;
        m_particleBuffer[idx + 5] = p.current_color.b;
        m_particleBuffer[idx + 6] = p.current_color.a;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_particleVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_particleBuffer.size() * sizeof(float), m_particleBuffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ExcitationRenderer::render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight) {
    if (m_particles.empty()) {
        ensureInitialized(const_cast<SimulationSettings&>(settings));
    }

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    m_particleShader->use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(float(viewportWidth) / float(viewportHeight));

    m_particleShader->setMat4("uModel", model);
    m_particleShader->setMat4("uView", view);
    m_particleShader->setMat4("uProjection", proj);
    m_particleShader->setFloat("uBaseSize", settings.point_base_size * 1.2f);
    m_particleShader->setFloat("uBrightness", settings.brightness * 1.5f);

    glBindVertexArray(m_particleVao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_particles.size()));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render Photon Wavepacket if active
    if (settings.photon_incoming || settings.photon_outgoing) {
        renderPhotonWavepacket(camera, settings);
    }
}

void ExcitationRenderer::renderPhotonWavepacket(const ArcballCamera& camera, const SimulationSettings& settings) {
    if (!m_photonShader) return;

    const int segments = 120;
    std::vector<float> lineData;
    lineData.reserve(segments * 7);

    float packetLength = 14.0f;
    float k_wavenumber = 2.5f;
    float sigma = 3.5f;

    for (int i = 0; i < segments; ++i) {
        float s = (float(i) / float(segments - 1) - 0.5f) * packetLength;
        float x = m_photonX + s;

        float envelope = std::exp(-(s * s) / (2.0f * sigma * sigma));
        float y = 2.2f * envelope * std::sin(k_wavenumber * s - m_animTime * 15.0f);
        float z = 0.0f;

        float alpha = envelope * 0.95f;

        lineData.push_back(x);
        lineData.push_back(y);
        lineData.push_back(z);
        lineData.push_back(settings.photon_color.r);
        lineData.push_back(settings.photon_color.g);
        lineData.push_back(settings.photon_color.b);
        lineData.push_back(alpha);
    }

    glBindVertexArray(m_photonVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_photonVbo);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(3.0f);

    m_photonShader->use();
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(1.333f);

    m_photonShader->setMat4("uModel", model);
    m_photonShader->setMat4("uView", view);
    m_photonShader->setMat4("uProjection", proj);

    glDrawArrays(GL_LINE_STRIP, 0, segments);

    glLineWidth(1.0f);
    glBindVertexArray(0);
}

} // namespace orbital
