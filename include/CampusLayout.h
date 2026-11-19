#pragma once
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// Station coordinates keep the existing interaction logic aligned with the machine.
// In campus coordinates the complete amenity row is on the left (x = -14),
// perpendicular to the VKU facade (z = -25), facing inward toward the courtyard.
namespace CampusLayout {
inline glm::vec3 toCampus(glm::vec3 p) { return {p.z - 14.0f, p.y, -p.x - 3.0f}; }
inline glm::mat4 campusToStation() {
    return glm::mat4(0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 0, -3, 0, 14, 1);
}
inline glm::mat4 stationToCampus() {
    return glm::mat4(0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -14, 0, -3, 1);
}
}
