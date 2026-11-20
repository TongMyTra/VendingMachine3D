#include "Camera.h"
#include "CampusLayout.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace {
constexpr float dockYaw = -90.0f;
glm::vec3 interactionPosition(float aspect) {
    // Leave room around the complete cabinet, including feet and header.
    const float distance = std::max(6.8f, 2.75f / (std::tan(glm::radians(30.0f)) * aspect) + 1.0f);
    return {0.0f, Camera::eyeHeight, distance};
}

bool walkable(const glm::vec3& p) {
    const auto campus = CampusLayout::toCampus(p);
    if (campus.x < -24 || campus.x > 24 || campus.z < -18 || campus.z > 25) return false;
    for (float x : {-9.0f, 9.0f})
        if (std::abs(p.x - x) < 2.7f && p.z > -0.95f && p.z < 1.0f) return false;
    for (float x : {-13.0f, 13.0f})
        if (glm::length(glm::vec2(p.x - x, p.z + 0.2f)) < 0.95f) return false;
    // Cabinet footprint expanded by the player's radius; axis-separated motion slides along it.
    if (p.x > -5.8f && p.x < -4.0f && p.z > -0.5f && p.z < 1.3f) return false;
    return !(p.x > -2.65f && p.x < 2.65f && p.z > -1.10f && p.z < 1.35f);
}
float smooth(float t) { return t * t * (3.0f - 2.0f * t); }
}

Camera::Camera(bool campusOverview) : campusOverview_(campusOverview) {
    if (campusOverview) {
        feetPosition_ = {-3.0f, eyeHeight, 27.0f};
        yaw_ = -67.0f;
        pitch_ = 5.0f;
    }
}

glm::vec3 Camera::forwardDirection() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    return {std::cos(yaw) * std::cos(pitch), std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)};
}

glm::vec3 Camera::position() const {
    if (presentationActive_) return presentationEye_;
    return feetPosition_ + glm::vec3(0.0f, bobOffset_ - pickupMotion_ * 0.25f + std::sin(drinkTime_ * 4.0f) * 0.008f * drinkMotion_, 0.0f);
}

glm::mat4 Camera::viewMatrix() const {
    if (presentationActive_) return glm::lookAt(presentationEye_, presentationTarget_, glm::vec3(0, 1, 0));
    return glm::lookAt(position(), position() + forwardDirection(), {0.0f, 1.0f, 0.0f});
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(campusOverview_ ? 66.0f : 60.0f), std::max(aspect, 0.01f), 0.08f, 100.0f);
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
    if (presentationActive_ || interacting_ || feetPosition_.z < 1.35f || feetPosition_.z > 8.5f ||
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
    const float dt = std::clamp(deltaTime, 0.0f, 0.25f);
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
        next.x += direction.x;
        if (walkable(next)) feetPosition_.x = next.x;
        next = feetPosition_;
        next.z += direction.z;
        if (walkable(next)) feetPosition_.z = next.z;
    }
    const float travelled = glm::length(feetPosition_ - before);
    bobPhase_ += travelled * 5.5f;
    const float targetBob = travelled > 0.0001f ? std::sin(bobPhase_) * 0.025f : 0.0f;
    bobOffset_ = glm::mix(bobOffset_, targetBob, 1.0f - std::exp(-14.0f * dt));
}

bool Camera::aimingAt(glm::vec3 point, float radius, float maxDistance) const {
    if (presentationActive_) return false;
    const glm::vec3 delta = point - position();
    const float t = glm::dot(delta, forwardDirection());
    return t > 0 && t < maxDistance && glm::length(delta - forwardDirection() * t) < radius;
}
void Camera::updatePresentation(float dt, bool cinematic, bool vending, float progress, glm::vec3 product) {
    const glm::vec3 baseEye = feetPosition_ + glm::vec3(0, bobOffset_, 0);
    const auto baseTarget = baseEye + forwardDirection();
    const bool active = external_ || (cinematic && vending);
    if (!active && !presentationActive_) return;
    if (!presentationActive_) {
        presentationEye_ = baseEye;
        presentationTarget_ = baseTarget;
        presentationActive_ = true;
    }
    glm::vec3 eye = baseEye, target = baseTarget;
    if (cinematic && vending) {
        const bool closeup = progress < 0.30f;
        eye = product + (closeup ? glm::vec3(0.60f, 0.45f, 1.65f) : glm::vec3(1.7f, 0.9f, 2.7f));
        target = product + glm::vec3(0, closeup ? -0.1f : 0, -0.08f);
    } else if (external_) {
        eye = campusOverview_ ? glm::vec3(-4.0f, 9.0f, 28.0f) : glm::vec3(7.0f, 5.4f, 10.5f);
        target = campusOverview_ ? glm::vec3(6.0f, 3.2f, 7.0f) : glm::vec3(0, 2.65f, 0);
    }
    const float t = 1 - std::exp(-7.0f * std::max(dt, 0.0f));
    presentationEye_ = glm::mix(presentationEye_, eye, t);
    presentationTarget_ = glm::mix(presentationTarget_, target, t);
    if (glm::length(presentationTarget_ - presentationEye_) < 0.1f) presentationTarget_ = presentationEye_ + forwardDirection();
    if (!active && glm::length(presentationEye_ - baseEye) < 0.005f && glm::length(presentationTarget_ - baseTarget) < 0.005f)
        presentationActive_ = false;
}

bool Camera::canUseTrash() const {
    if (presentationActive_) return false;
    // Test the whole visible bin, rather than a small point hidden beneath its hood.
    const glm::vec3 minimum{-5.6f, -0.10f, -0.20f};
    const glm::vec3 maximum{-4.2f, 2.16f, 1.03f};
    const auto origin = position();
    const auto direction = forwardDirection();
    float near = 0.0f, far = 4.5f;
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(direction[axis]) < 0.0001f) {
            if (origin[axis] < minimum[axis] || origin[axis] > maximum[axis]) return false;
        } else {
            float a = (minimum[axis] - origin[axis]) / direction[axis];
            float b = (maximum[axis] - origin[axis]) / direction[axis];
            if (a > b) std::swap(a, b);
            near = std::max(near, a);
            far = std::min(far, b);
            if (near > far) return false;
        }
    }
    return far >= near;
}
