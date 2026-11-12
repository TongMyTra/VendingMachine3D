#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::vec3 Camera::position() const {
    const float yawRadians = glm::radians(yaw_);
    const float pitchRadians = glm::radians(pitch_);
    return {distance_ * std::sin(yawRadians) * std::cos(pitchRadians),
            2.0f + distance_ * std::sin(pitchRadians),
            distance_ * std::cos(yawRadians) * std::cos(pitchRadians)};
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position(), glm::vec3(0.0f, 2.0f, 0.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(42.0f), aspect, 0.1f, 100.0f);
}

void Camera::orbit(float deltaX, float deltaY) {
    yaw_ += deltaX * 0.25f;
    pitch_ = std::clamp(pitch_ + deltaY * 0.2f, -8.0f, 38.0f);
    if (yaw_ > 360.0f || yaw_ < -360.0f) {
        yaw_ = std::fmod(yaw_, 360.0f);
    }
}

void Camera::zoom(float amount) {
    distance_ = std::clamp(distance_ - amount * 0.35f, 5.0f, 11.0f);
}
