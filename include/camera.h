#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace orbital {

class ArcballCamera {
public:
    glm::vec3 target{0.0f, 0.0f, 0.0f};
    float radius = 45.0f;
    float azimuth = 0.8f;          // Polar angle in horizontal plane (radians)
    float elevation = 1.2f;        // Vertical angle from up-axis (radians)
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    float orbitSpeed = 0.006f;
    float panSpeed = 0.02f;
    float zoomSpeed = 2.0f;

    bool autoRotate = false;
    float autoRotateSpeed = 0.2f; // radians/sec

    bool isDraggingOrbit = false;
    bool isDraggingPan = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    ArcballCamera() = default;

    glm::vec3 getPosition() const {
        float clampedElev = std::clamp(elevation, 0.01f, static_cast<float>(M_PI) - 0.01f);
        return target + glm::vec3(
            radius * std::sin(clampedElev) * std::cos(azimuth),
            radius * std::cos(clampedElev),
            radius * std::sin(clampedElev) * std::sin(azimuth)
        );
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 getProjectionMatrix(float aspectRatio) const {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    void update(float deltaTime) {
        if (autoRotate && !isDraggingOrbit) {
            azimuth += autoRotateSpeed * deltaTime;
            if (azimuth > 2.0f * M_PI) azimuth -= 2.0f * M_PI;
        }
    }

    void onMouseButton(int button, int action, double x, double y) {
        if (action == 1 /* GLFW_PRESS */) {
            lastMouseX = x;
            lastMouseY = y;
            if (button == 0 /* GLFW_MOUSE_BUTTON_LEFT */) {
                isDraggingOrbit = true;
            } else if (button == 1 /* GLFW_MOUSE_BUTTON_RIGHT */) {
                isDraggingPan = true;
            }
        } else if (action == 0 /* GLFW_RELEASE */) {
            if (button == 0) isDraggingOrbit = false;
            if (button == 1) isDraggingPan = false;
        }
    }

    void onMouseMove(double x, double y) {
        float dx = static_cast<float>(x - lastMouseX);
        float dy = static_cast<float>(y - lastMouseY);
        lastMouseX = x;
        lastMouseY = y;

        if (isDraggingOrbit) {
            azimuth -= dx * orbitSpeed;
            elevation -= dy * orbitSpeed;
            elevation = std::clamp(elevation, 0.01f, static_cast<float>(M_PI) - 0.01f);
        } else if (isDraggingPan) {
            // Pan relative to camera orientation
            glm::vec3 pos = getPosition();
            glm::vec3 forward = glm::normalize(target - pos);
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            glm::vec3 up = glm::cross(right, forward);

            float scale = radius * 0.0015f;
            target += (-right * dx + up * dy) * scale;
        }
    }

    void onScroll(double yoffset) {
        radius -= static_cast<float>(yoffset) * zoomSpeed * (radius * 0.05f);
        radius = std::clamp(radius, 2.0f, 400.0f);
    }

    void reset(float newRadius = 45.0f) {
        target = glm::vec3(0.0f, 0.0f, 0.0f);
        radius = newRadius;
        azimuth = 0.8f;
        elevation = 1.2f;
    }
};

} // namespace orbital

#endif // CAMERA_H
