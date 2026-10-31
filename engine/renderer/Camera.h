#pragma once

#include "engine/core/Math.h"

namespace br {

struct Camera {
    glm::vec3 position{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f}; // -Z forward, +Y up (OpenGL convention)
    float fovYDegrees = 75.0f;
    float nearPlane = 0.05f;
    float farPlane = 400.0f;
    float aspect = 16.0f / 9.0f;

    static glm::quat orientationFromYawPitch(float yawRadians, float pitchRadians, float rollRadians = 0.0f) {
        const glm::quat yaw = glm::angleAxis(yawRadians, glm::vec3(0, 1, 0));
        const glm::quat pitch = glm::angleAxis(pitchRadians, glm::vec3(1, 0, 0));
        const glm::quat roll = glm::angleAxis(rollRadians, glm::vec3(0, 0, 1));
        return yaw * pitch * roll;
    }

    glm::vec3 forward() const { return orientation * glm::vec3(0, 0, -1); }
    glm::vec3 right() const { return orientation * glm::vec3(1, 0, 0); }
    glm::vec3 up() const { return orientation * glm::vec3(0, 1, 0); }

    glm::mat4 view() const {
        const glm::mat4 r = glm::mat4_cast(glm::conjugate(orientation));
        return r * glm::translate(glm::mat4(1.0f), -position);
    }
    glm::mat4 projection() const {
        return glm::perspective(glm::radians(fovYDegrees), aspect, nearPlane, farPlane);
    }
    glm::mat4 viewProjection() const { return projection() * view(); }
};

} // namespace br
