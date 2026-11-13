#include "HeldDrink.h"
#include <algorithm>
#include <cmath>

bool HeldDrink::pickUp(const Product& product, glm::vec3 trayPosition) {
    if (hasProduct()) return false;
    product_ = product;
    trayPosition_ = trayPosition;
    capacity_ = product.type == ProductType::Can ? 330.0f : product.type == ProductType::Bottle ? 500.0f : 400.0f;
    remaining_ = capacity_;
    pickupTime_ = openingTime_ = drinkBlend_ = gulpTime_ = time_ = 0;
    opening_ = opened_ = openingSound_ = false;
    placed_ = inspecting_ = tossing_ = pickupFromTable_ = false;
    inspectBlend_ = inspectYaw_ = inspectPitch_ = 0;
    inspectDistance_ = 0.90f;
    return true;
}
float HeldDrink::openingDuration() const {
    return holding() && product_->type == ProductType::Bottle ? 2.2f : 1.6f;
}
bool HeldDrink::open() {
    if (!holding() || inspecting_ || pickingUp() || opening_ || opened_) return false;
    opening_ = true;
    openingTime_ = 0;
    return true;
}
float HeldDrink::pickupProgress() const { return std::clamp(pickupTime_ / 0.75f, 0.0f, 1.0f); }
float HeldDrink::openingProgress() const {
    return opened_ ? 1.0f : std::clamp(openingTime_ / openingDuration(), 0.0f, 1.0f);
}
void HeldDrink::update(float dt, bool drinkHeld) {
    dt = std::max(dt, 0.0f);
    if (tossing_) {
        tossTime_ += dt;
        if (tossTime_ >= 0.9f) { product_.reset(); tossing_ = false; }
        return;
    }
    if (placed_) { placementTime_ = std::min(placementTime_ + dt, 0.5f); return; }
    if (!holding()) return;
    time_ += dt;
    inspectBlend_ += ((inspecting_ ? 1.0f : 0.0f) - inspectBlend_) * (1 - std::exp(-9 * dt));
    pickupTime_ = std::min(pickupTime_ + dt, 0.75f);
    if (opening_) {
        openingTime_ += dt;
        if (!openingSound_ && openingProgress() >= 0.45f) {
            sound_ = product_->type == ProductType::Can ? 6 : 7;
            openingSound_ = true;
        }
        if (openingTime_ >= openingDuration()) { opening_ = false; opened_ = true; }
    }
    const bool canDrink = drinkHeld && !inspecting_ && opened_ && !opening_ && !pickingUp() && remaining_ > 0;
    const float target = canDrink ? 1.0f : 0.0f;
    drinkBlend_ += (target - drinkBlend_) * (1 - std::exp(-7.0f * dt));
    if (canDrink && drinkBlend_ > 0.75f) {
        remaining_ = std::max(0.0f, remaining_ - 55.0f * dt);
        gulpTime_ -= dt;
        if (gulpTime_ <= 0) { sound_ = 8; gulpTime_ = 0.65f; }
    } else gulpTime_ = 0;
}
bool HeldDrink::discardEmpty() {
    if (!empty() || drinkBlend_ > 0.05f) return false;
    product_.reset();
    return true;
}
int HeldDrink::takeSound() { const int result = sound_; sound_ = 0; return result; }

bool HeldDrink::toggleInspect() {
    if (!holding() || opening_ || pickingUp() || drinkBlend_ > 0.1f) return false;
    inspecting_ = !inspecting_;
    return true;
}
void HeldDrink::inspectRotate(float x, float y) {
    if (!inspecting_) return;
    inspectYaw_ = std::remainder(inspectYaw_ + x * 0.45f, 360.0f);
    inspectPitch_ = std::clamp(inspectPitch_ + y * 0.35f, -85.0f, 85.0f);
}
void HeldDrink::inspectZoom(float amount) {
    if (inspecting_) inspectDistance_ = std::clamp(inspectDistance_ - amount * 0.06f, 0.65f, 1.5f);
}
bool HeldDrink::putDown(glm::vec3 position) {
    if (!holding() || opening_ || pickingUp() || inspecting_ || drinkBlend_ > 0.05f) return false;
    placed_ = true;
    placementTime_ = 0;
    placedPosition_ = position;
    return true;
}
bool HeldDrink::takePlaced() {
    if (!placed_ || placementTime_ < 0.5f) return false;
    placed_ = false;
    pickupFromTable_ = true;
    trayPosition_ = placedPosition_;
    pickupTime_ = 0;
    return true;
}

bool HeldDrink::throwAway(glm::vec3 target) {
    if (!empty() || inspecting_ || opening_) return false;
    drinkBlend_ = 0;
    tossing_ = true;
    tossTime_ = 0;
    trashTarget_ = target;
    return true;
}
