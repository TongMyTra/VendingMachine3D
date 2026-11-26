#pragma once
#include "CampusLayout.h"
#include <glm/geometric.hpp>
#include <cmath>

namespace StudentCrowd {
constexpr int count = 4;
struct Pose { glm::vec3 position; float heading; float gait; };
inline Pose sample(int index, float time) {
    // One pair circles the courtyard; the other stays on the far side of the fountain plaza.
    const float direction = index % 2 ? -1.0f : 1.0f;
    const float angle = index * 2.399963f + time * 0.34f * direction;
    const bool farSideLane = index % 2 == 0;
    const float centerX = farSideLane ? 11.2f : -2.0f;
    const float centerZ = farSideLane ? 6.0f : -8.0f;
    const float radiusX = farSideLane ? 1.4f : 5.0f;
    const float radiusZ = farSideLane ? 1.3f : 3.0f;
    const glm::vec3 campus{centerX + radiusX * std::cos(angle), -0.09f, centerZ + radiusZ * std::sin(angle)};
    const auto local = glm::vec3(CampusLayout::campusToStation() * glm::vec4(campus, 1));
    const glm::vec3 tangent = glm::vec3(CampusLayout::campusToStation() *
        glm::vec4(-radiusX * std::sin(angle) * direction, 0, radiusZ * std::cos(angle) * direction, 0));
    return {local, std::atan2(tangent.x, tangent.z), time * 6.2f + index * 1.7f};
}
}
