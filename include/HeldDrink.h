#pragma once
#include "Product.h"
#include <glm/vec3.hpp>
#include <optional>
#include <algorithm>

class HeldDrink {
public:
    bool pickUp(const Product& product, glm::vec3 trayPosition);
    bool open();
    bool toggleInspect();
    void inspectRotate(float x, float y);
    void inspectZoom(float amount);
    bool inspecting() const { return inspecting_; }
    float inspectBlend() const { return inspectBlend_; }
    float inspectYaw() const { return inspectYaw_; }
    float inspectPitch() const { return inspectPitch_; }
    float inspectDistance() const { return inspectDistance_; }
    bool putDown(glm::vec3 position);
    bool takePlaced();
    bool throwAway(glm::vec3 target);
    bool tossing() const { return tossing_; }
    float tossProgress() const { return tossTime_ / 0.9f; }
    glm::vec3 trashTarget() const { return trashTarget_; }
    float placementProgress() const { return std::min(placementTime_ / 0.5f, 1.0f); }
    bool pickupFromTable() const { return pickupFromTable_; }
    bool placed() const { return placed_; }
    glm::vec3 placedPosition() const { return placedPosition_; }
    bool hasProduct() const { return product_.has_value(); }
    void update(float dt, bool drinkHeld);
    bool discardEmpty();
    bool holding() const { return product_.has_value() && !placed_ && !tossing_; }
    bool pickingUp() const { return holding() && pickupTime_ < 0.75f; }
    bool opening() const { return opening_; }
    bool opened() const { return opened_; }
    bool empty() const { return holding() && remaining_ <= 0; }
    const Product& product() const { return product_.value(); }
    float pickupProgress() const;
    glm::vec3 trayPosition() const { return trayPosition_; }
    float openingProgress() const;
    float drinkBlend() const { return drinkBlend_; }
    float remaining() const { return remaining_; }
    float capacity() const { return capacity_; }
    float time() const { return time_; }
    int takeSound();
private:
    bool tossing_{};
    float tossTime_{};
    glm::vec3 trashTarget_{};
    bool pickupFromTable_{};
    float placementTime_{};
    bool placed_{};
    glm::vec3 placedPosition_{};
    bool inspecting_{};
    float inspectBlend_{};
    float inspectYaw_{};
    float inspectPitch_{};
    float inspectDistance_{0.90f};
    std::optional<Product> product_;
    glm::vec3 trayPosition_{};
    float pickupTime_{};
    float openingTime_{};
    float time_{};
    float capacity_{};
    float remaining_{};
    float drinkBlend_{};
    float gulpTime_{};
    bool opening_{};
    bool opened_{};
    bool openingSound_{};
    int sound_{};
    float openingDuration() const;
};
