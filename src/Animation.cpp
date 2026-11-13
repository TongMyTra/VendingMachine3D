#include "Animation.h"
#include <algorithm>
#include <glm/common.hpp>
#include <cmath>

namespace {
float smooth(float t) { t = std::clamp(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }
}
float Animation::fallDuration() const {
    const float height = 4.33f - (productIndex_ / 4) * 1.32f;
    return std::sqrt(2.0f * std::max(height - 0.9f, 0.0f) / 8.0f);
}
float Animation::duration() const { return 0.8f + fallDuration() + 0.65f; }
void Animation::start(int productIndex) {
    if (active_ || delivered_ || productIndex < 0 || productIndex >= 12) return;
    productIndex_ = productIndex;
    elapsed_ = 0;
    active_ = true;
    arrival_ = false;
    sounds_ |= 2;
}
void Animation::press(int index) {
    if (active_ || pressing() || index < 0 || index >= 12) return;
    pressIndex_ = index;
    pressElapsed_ = 0;
}
void Animation::update(float dt) {
    dt = std::max(dt, 0.0f);
    if (pressing()) {
        const float previous = pressElapsed_;
        pressElapsed_ += dt;
        if (previous < 0.42f && pressElapsed_ >= 0.42f) sounds_ |= 1;
        if (pressElapsed_ >= 0.95f) pressIndex_ = -1;
    }
    if (!active_) return;
    const float previous = elapsed_;
    elapsed_ = std::min(elapsed_ + dt, duration());
    const float impact = 0.8f + fallDuration();
    if (previous < impact && elapsed_ >= impact) sounds_ |= 4;
    if (elapsed_ >= duration()) {
        active_ = false;
        delivered_ = true;
        arrival_ = true;
        sounds_ |= 8;
    }
}
bool Animation::active() const { return active_; }
int Animation::productIndex() const { return productIndex_; }
float Animation::progress() const { return productIndex_ >= 0 ? std::clamp(elapsed_ / duration(), 0.0f, 1.0f) : 0; }
float Animation::pressProgress() const { return std::clamp(pressElapsed_ / 0.95f, 0.0f, 1.0f); }
float Animation::buttonDepth(int index) const {
    if (index != pressIndex_) return 0;
    return 0.035f * smooth((pressElapsed_ - 0.32f) / 0.10f) * (1 - smooth((pressElapsed_ - 0.57f) / 0.16f));
}
glm::vec3 Animation::deliveryPosition(glm::vec3 shelf) const {
    if (elapsed_ < 0.8f) { shelf.z = glm::mix(shelf.z, 1.0f, smooth(elapsed_ / 0.8f)); return shelf; }
    const float fall = std::min(elapsed_ - 0.8f, fallDuration());
    shelf.y -= 4.0f * fall * fall;
    shelf.z = 1.0f;
    if (elapsed_ < 0.8f + fallDuration()) return shelf;
    const float t = smooth((elapsed_ - 0.8f - fallDuration()) / 0.65f);
    auto result = glm::mix(shelf, glm::vec3(-0.43f, 0.49f, 1.17f), t);
    result.y += 0.075f * std::sin(t * 3.14159265f); // Small damped chute bounce.
    return result;
}
float Animation::deliveryRotation() const {
    if (elapsed_ < 0.8f) return 0;
    const float fall = std::min((elapsed_ - 0.8f) / fallDuration(), 1.0f);
    const float slide = smooth((elapsed_ - 0.8f - fallDuration()) / 0.65f);
    return 18.0f * fall + 72.0f * slide;
}
float Animation::dropOffset() const {
    const glm::vec3 shelf{0, 4.33f - (productIndex_ / 4) * 1.32f, 0.43f};
    return productIndex_ >= 0 ? shelf.y - deliveryPosition(shelf).y : 0;
}
bool Animation::takeArrival() { const bool value = arrival_; arrival_ = false; return value; }
int Animation::takeSound() {
    for (int i = 0; i < 5; ++i) if (sounds_ & (1 << i)) { sounds_ &= ~(1 << i); return i + 1; }
    return 0;
}
