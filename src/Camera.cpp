#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr float dockYaw = -90.0f;
glm::vec3 interactionPosition(float aspect) {
    // Leave room around the complete cabinet, including feet and header.
    const float distance = std::max(7.6f, 2.75f / (std::tan(glm::radians(30.0f)) * aspect) + 1.0f);
    return {0.0f, Camera::eyeHeight, distance};
}

bool walkable(const glm::vec3& p) {
    // Cabinet footprint expanded by the player's radius; axis-separated motion slides along it.
    return !(p.x > -2.65f && p.x < 2.65f && p.z > -1.10f && p.z < 1.35f);
}
float smooth(float t) { return t * t * (3.0f - 2.0f * t); }
}

glm::vec3 Camera::forwardDirection() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    return {std::cos(yaw) * std::cos(pitch), std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)};
}

glm::vec3 Camera::position() const {
    return feetPosition_ + glm::vec3(0.0f, bobOffset_, 0.0f);
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position(), position() + forwardDirection(), {0.0f, 1.0f, 0.0f});
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(60.0f), std::max(aspect, 0.01f), 0.08f, 100.0f);
}

void Camera::look(float deltaX, float deltaY) {
    if (interacting_ && (!paymentFocus_ || transitioning())) return;
    yaw_ = std::remainder(yaw_ + deltaX * 0.10f, 360.0f);
    pitch_ = std::clamp(pitch_ - deltaY * 0.10f, -85.0f, 85.0f);
}

void Camera::setInteractionAspect(float aspect) {
    interactionAspect_ = std::max(aspect, 0.1f);
}

void Camera::focusPayment(bool enabled) {
    if (!interacting_ || enabled == paymentFocus_) return;
    paymentFocus_ = enabled;
    transitionStart_ = position();
    startYaw_ = yaw_;
    startPitch_ = pitch_;
    transitionTime_ = 0.0f;
}

bool Camera::aimingAtPaymentSlot() const {
    if (!paymentFocus_ || transitioning()) return false;
    const auto direction = forwardDirection();
    if (direction.z >= -0.001f) return false;
    const float distance = (0.80f - position().z) / direction.z;
    if (distance < 0.0f || distance > 3.0f) return false;
    const auto hit = position() + direction * distance;
    return std::abs(hit.x - 1.69f) < 0.29f && std::abs(hit.y - 2.55f) < 0.14f;
}

float Camera::interactionBlend() const {
    return interacting_ ? smooth(std::clamp(transitionTime_ / transitionDuration, 0.0f, 1.0f)) : 0.0f;
}

bool Camera::canInteract() const {
    if (interacting_ || feetPosition_.z < 1.35f || feetPosition_.z > 8.5f ||
        std::abs(feetPosition_.x) > 3.25f) return false;
    const glm::vec3 toPanel = glm::normalize(glm::vec3(1.69f, 3.9f, 0.8f) - position());
    return glm::dot(forwardDirection(), toPanel) > 0.35f;
}

bool Camera::beginInteraction() {
    if (!canInteract()) return false;
    interacting_ = true;
    transitionTime_ = 0.0f;
    transitionStart_ = position();
    bobOffset_ = 0.0f;
    startYaw_ = yaw_;
    startPitch_ = pitch_;
    return true;
}

void Camera::endInteraction() {
    interacting_ = false;
    paymentFocus_ = false;
    transitionTime_ = 0.0f;
    bobOffset_ = 0.0f;
    // Continue walking from the current pose, including when cancelling mid-transition.
}

void Camera::update(float deltaTime, float forward, float strafe, bool sprint) {
    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    if (interacting_) {
        const bool wasTransitioning = transitioning();
        transitionTime_ = std::min(transitionTime_ + dt, transitionDuration);
        const float t = interactionBlend();
        if (paymentFocus_ && !wasTransitioning) return;
        const auto dockPosition = paymentFocus_ ? glm::vec3(1.25f, eyeHeight, 2.4f) : interactionPosition(interactionAspect_);
        const float targetYaw = paymentFocus_ ? -74.8f : dockYaw;
        const float dockPitch = paymentFocus_ ? -41.3f : glm::degrees(std::atan2(2.65f - eyeHeight, dockPosition.z));
        feetPosition_ = glm::mix(transitionStart_, dockPosition, t);
        yaw_ = startYaw_ + std::remainder(targetYaw - startYaw_, 360.0f) * t;
        pitch_ = glm::mix(startPitch_, dockPitch, t);
        return;
    }
    const float yaw = glm::radians(yaw_);
    glm::vec3 direction = glm::vec3(std::cos(yaw), 0.0f, std::sin(yaw)) * forward +
                          glm::vec3(-std::sin(yaw), 0.0f, std::cos(yaw)) * strafe;
    const glm::vec3 before = feetPosition_;
    if (glm::length(direction) > 0.001f) {
        direction = glm::normalize(direction) * (sprint ? 7.0f : 3.5f) * dt;
        glm::vec3 next = feetPosition_;
        next.x = std::clamp(next.x + direction.x, -25.0f, 25.0f);
        if (walkable(next)) feetPosition_.x = next.x;
        next = feetPosition_;
        next.z = std::clamp(next.z + direction.z, -25.0f, 25.0f);
        if (walkable(next)) feetPosition_.z = next.z;
    }
    const float travelled = glm::length(feetPosition_ - before);
    bobPhase_ += travelled * 5.5f;
    const float targetBob = travelled > 0.0001f ? std::sin(bobPhase_) * 0.025f : 0.0f;
    bobOffset_ = glm::mix(bobOffset_, targetBob, 1.0f - std::exp(-14.0f * dt));
}
