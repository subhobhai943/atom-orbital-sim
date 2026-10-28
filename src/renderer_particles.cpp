#include "../include/renderer_particles.h"
#include <iostream>
#include <cmath>

namespace orbital {

ParticleRenderer::ParticleRenderer() = default;

ParticleRenderer::~ParticleRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_nucleusVao) glDeleteVertexArrays(1, &m_nucleusVao);
    if (m_nucleusVbo) glDeleteBuffers(1, &m_nucleusVbo);
    if (m_nucleusEbo) glDeleteBuffers(1, &m_nucleusEbo);
}

bool ParticleRenderer::init() {
    m_shader = std::make_unique<Shader>();
    if (!m_shader->loadFromFiles("shaders/particle.vert", "shaders/particle.frag")) {
        // Fallback to internal shader strings if shader files aren't found in current directory
        const std::string vertSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uBaseSize;
uniform bool uEnableClip;
uniform vec4 uClipPlane;
out vec4 vColor;
out float vDist;
void main() {
    if (uEnableClip) {
        float dist = dot(aPos, uClipPlane.xyz) + uClipPlane.w;
        if (dist < 0.0) {
            gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
            return;
        }
    }
    vec4 viewPos = uView * uModel * vec4(aPos, 1.0);
    gl_Position = uProjection * viewPos;
    float depth = max(-viewPos.z, 0.1);
    gl_PointSize = clamp(uBaseSize * (600.0 / depth), 1.5, 64.0);
    vColor = aColor;
    vDist = depth;
}
)";
        const std::string fragSrc = R"(#version 330 core
in vec4 vColor;
in float vDist;
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
        if (!m_shader->loadFromStrings(vertSrc, fragSrc)) {
            std::cerr << "Failed to initialize particle shaders!\n";
            return false;
        }
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    // Initial allocation
    glBufferData(GL_ARRAY_BUFFER, 500000 * sizeof(ParticleVertex), nullptr, GL_DYNAMIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)0);

    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleVertex), (void*)(sizeof(glm::vec3)));

    glBindVertexArray(0);

    initNucleusMesh();
    return true;
}

void ParticleRenderer::initNucleusMesh() {
    m_nucleusShader = std::make_unique<Shader>();
    if (!m_nucleusShader->loadFromFiles("shaders/nucleus.vert", "shaders/nucleus.frag")) {
        const std::string vertSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vNormal;
out vec3 vFragPos;
void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uProjection * uView * worldPos;
}
)";
        const std::string fragSrc = R"(#version 330 core
in vec3 vNormal;
in vec3 vFragPos;
uniform vec3 uColor;
uniform vec3 uCameraPos;
uniform float uPulse;
out vec4 FragColor;
void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(0.5, 1.0, 0.7));
    vec3 V = normalize(uCameraPos - vFragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0);
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 ambient = 0.25 * uColor;
    vec3 diffuse = diff * uColor;
    vec3 specular = spec * vec3(1.0);
    vec3 rim = fresnel * (uColor * 0.8 + vec3(0.2)) * (0.8 + 0.2 * uPulse);
    FragColor = vec4(ambient + diffuse + specular + rim, 1.0);
}
)";
        m_nucleusShader->loadFromStrings(vertSrc, fragSrc);
    }

    // Generate UV Sphere
    const int stacks = 20;
    const int slices = 30;
    std::vector<float> vertices; // pos(3), normal(3)
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = float(M_PI) * float(i) / float(stacks);
        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * float(M_PI) * float(j) / float(slices);

            float x = std::sin(phi) * std::cos(theta);
            float y = std::cos(phi);
            float z = std::sin(phi) * std::sin(theta);

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int first = (i * (slices + 1)) + j;
            int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    m_nucleusIndexCount = static_cast<int>(indices.size());

    glGenVertexArrays(1, &m_nucleusVao);
    glGenBuffers(1, &m_nucleusVbo);
    glGenBuffers(1, &m_nucleusEbo);

    glBindVertexArray(m_nucleusVao);

    glBindBuffer(GL_ARRAY_BUFFER, m_nucleusVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_nucleusEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void ParticleRenderer::generateParticles(SimulationSettings& settings) {
    m_particles.clear();
    m_particles.reserve(settings.particle_count);
    m_gpuBuffer.resize(settings.particle_count);

    float intensity_scale = 1.5f * static_cast<float>(std::pow(5.0, settings.n));

    for (int i = 0; i < settings.particle_count; ++i) {
        float r = static_cast<float>(settings.radial_sampler.sample(settings.n, settings.l, settings.rng));
        float theta = static_cast<float>(settings.theta_sampler.sample(settings.l, settings.m, settings.rng));
        float phi = static_cast<float>(sample_phi(settings.m, settings.use_real_orbitals, settings.rng));

        Vec3 pos_sph = spherical_to_cartesian(r, theta, phi);
        glm::vec3 pos(pos_sph.x, pos_sph.y, pos_sph.z);

        Color4 col;
        if (settings.palette == ColorPalette::QuantumPhase) {
            float psi = static_cast<float>(wavefunction_real(r, theta, phi, settings.n, settings.l, settings.m));
            col = palette_phase(psi, 0.15f);
        } else {
            double prob_density = probability_density(r, theta, phi, settings.n, settings.l, settings.m, settings.use_real_orbitals);
            float norm_intensity = std::clamp(static_cast<float>(prob_density * intensity_scale), 0.0f, 1.0f);
            col = map_orbital_color(norm_intensity, settings.palette);
        }

        glm::vec4 glm_col(col.r, col.g, col.b, col.a);

        m_particles.push_back({pos, pos, glm_col, r, theta, phi});
        m_gpuBuffer[i] = {pos, glm_col};
    }

    // Upload to GPU
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_gpuBuffer.size() * sizeof(ParticleVertex), m_gpuBuffer.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleRenderer::update(float deltaTime, SimulationSettings& settings) {
    // If probability flow is enabled and m != 0, update phi angles and positions
    if (settings.enable_probability_flow && settings.m != 0 && !settings.use_real_orbitals) {
        float speed_scale = settings.flow_speed * deltaTime * 3.0f;

        for (size_t i = 0; i < m_particles.size(); ++i) {
            auto& p = m_particles[i];
            float sin_theta = std::sin(p.theta);
            if (std::abs(sin_theta) < 1e-3f) sin_theta = (sin_theta >= 0.0f ? 1e-3f : -1e-3f);

            float dphi = (settings.m / (p.radius * sin_theta)) * speed_scale;
            p.phi += dphi;
            if (p.phi > 2.0f * float(M_PI)) p.phi -= 2.0f * float(M_PI);
            else if (p.phi < 0.0f) p.phi += 2.0f * float(M_PI);

            Vec3 new_pos = spherical_to_cartesian(p.radius, p.theta, p.phi);
            p.current_pos = glm::vec3(new_pos.x, new_pos.y, new_pos.z);
            m_gpuBuffer[i].pos = p.current_pos;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_gpuBuffer.size() * sizeof(ParticleVertex), m_gpuBuffer.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void ParticleRenderer::render(const ArcballCamera& camera, const SimulationSettings& settings, int viewportWidth, int viewportHeight) {
    if (m_particles.empty()) return;

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    // Additive glowing particle blending
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE); // Particles don't write to depth buffer for glowing transparency

    m_shader->use();

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(float(viewportWidth) / float(viewportHeight));

    m_shader->setMat4("uModel", model);
    m_shader->setMat4("uView", view);
    m_shader->setMat4("uProjection", proj);
    m_shader->setFloat("uBaseSize", settings.point_base_size);
    m_shader->setFloat("uBrightness", settings.brightness);
    m_shader->setBool("uEnableClip", settings.enable_slice);

    // Compute clip plane equation (nx, ny, nz, d)
    glm::vec4 clipPlane(0.0f, 1.0f, 0.0f, -settings.slice_height);
    if (settings.slice_axis == 0) clipPlane = glm::vec4(1.0f, 0.0f, 0.0f, -settings.slice_height);
    else if (settings.slice_axis == 2) clipPlane = glm::vec4(0.0f, 0.0f, 1.0f, -settings.slice_height);
    m_shader->setVec4("uClipPlane", clipPlane);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_particles.size()));
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render Nucleus at origin
    if (settings.show_nucleus) {
        static float timeAcc = 0.0f;
        timeAcc += 0.016f;
        renderNucleus(camera, settings, timeAcc);
    }
}

void ParticleRenderer::renderNucleus(const ArcballCamera& camera, const SimulationSettings& settings, float time) {
    if (!m_nucleusShader || !m_nucleusVao) return;

    glEnable(GL_DEPTH_TEST);
    m_nucleusShader->use();

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(1.333f);
    m_nucleusShader->setMat4("uView", view);
    m_nucleusShader->setMat4("uProjection", proj);
    m_nucleusShader->setVec3("uCameraPos", camera.getPosition());
    m_nucleusShader->setFloat("uPulse", 0.5f + 0.5f * std::sin(time * 3.0f));

    glBindVertexArray(m_nucleusVao);

    // Render Protons (Red) and Neutrons (Cyan/White) clustered at center
    float scale = 0.65f * settings.nucleus_scale;
    const glm::vec3 cluster[4] = {
        glm::vec3( 0.35f,  0.25f,  0.0f) * scale,
        glm::vec3(-0.35f,  0.25f,  0.0f) * scale,
        glm::vec3( 0.0f,  -0.35f,  0.3f) * scale,
        glm::vec3( 0.0f,  -0.10f, -0.3f) * scale
    };
    const glm::vec3 colors[4] = {
        glm::vec3(0.95f, 0.20f, 0.15f), // Proton
        glm::vec3(0.95f, 0.20f, 0.15f), // Proton
        glm::vec3(0.30f, 0.65f, 0.95f), // Neutron
        glm::vec3(0.30f, 0.65f, 0.95f)  // Neutron
    };

    for (int i = 0; i < 4; ++i) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), cluster[i]);
        model = glm::scale(model, glm::vec3(scale * 0.7f));
        m_nucleusShader->setMat4("uModel", model);
        m_nucleusShader->setVec3("uColor", colors[i]);
        glDrawElements(GL_TRIANGLES, m_nucleusIndexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}

} // namespace orbital
