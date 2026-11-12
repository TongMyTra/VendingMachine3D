#include "Animation.h"

#include <algorithm>

namespace {
constexpr float duration = 1.0f;
}

void Animation::start(int productIndex) {
    productIndex_ = productIndex;
    elapsed_ = 0.0f;
    active_ = true;
}

void Animation::update(float deltaTime) {
    if (!active_) {
        return;
    }
    elapsed_ += std::max(deltaTime, 0.0f);
    if (elapsed_ >= duration) {
        active_ = false;
        productIndex_ = -1;
    }
}

bool Animation::active() const {
    return active_;
}

int Animation::productIndex() const {
    return productIndex_;
}

float Animation::dropOffset() const {
    if (!active_) {
        return 0.0f;
    }
    const float progress = std::min(elapsed_ / duration, 1.0f);
    const float shelf = static_cast<float>(productIndex_ / 3);
    const float fallDistance = 4.06f - shelf * 0.86f;
    return progress * progress * fallDistance;
}

float Animation::progress() const {
    if (!active_) {
        return 0.0f;
    }
    return std::min(elapsed_ / duration, 1.0f);
}
