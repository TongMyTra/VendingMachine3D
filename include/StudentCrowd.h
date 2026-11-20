#pragma once
#include "CampusLayout.h"
#include <glm/geometric.hpp>
#include <cmath>

namespace StudentCrowd {
constexpr int count = 8;
struct Pose { glm::vec3 position; float heading; float gait; };
inline Pose sample(int index, float time) {
    // Separate elliptical lanes in the courtyard keep the vending row unobstructed.
    const float direction = index % 2 ? -1.0f : 1.0f;
    const float angle = index * 2.399963f + time * 0.34f * direction;
    const float centerX = index < 4 ? -2.0f : 9.0f;
    const float centerZ = index % 2 ? -8.0f : 4.0f;
    const glm::vec3 campus{centerX + 5.0f * std::cos(angle), -0.09f, centerZ + 3.0f * std::sin(angle)};
    const auto local = glm::vec3(CampusLayout::campusToStation() * glm::vec4(campus, 1));
    const glm::vec3 tangent = glm::vec3(CampusLayout::campusToStation() *
        glm::vec4(-5.0f * std::sin(angle) * direction, 0, 3.0f * std::cos(angle) * direction, 0));
    return {local, std::atan2(tangent.x, tangent.z), time * 6.2f + index * 1.7f};
}
}
