#include "StudentCrowd.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    for (int index = 0; index < StudentCrowd::count; ++index) {
        const auto initial = StudentCrowd::sample(index, 0);
        const auto later = StudentCrowd::sample(index, 0.1f);
        assert(glm::length(initial.position - later.position) > 0.05f);
        assert(glm::length(initial.position - later.position) < 0.25f);
        assert(later.gait > initial.gait);
        for (int tick = 0; tick < 1000; ++tick) {
            const auto pose = StudentCrowd::sample(index, tick * 0.1f);
            const auto campus = CampusLayout::toCampus(pose.position);
            assert(std::isfinite(pose.heading));
            assert(campus.x > -8 && campus.x < 15);
            assert(campus.z > -12 && campus.z < 8);
            assert(campus.x > -12); // Never enter the left-side vending row.
            const auto repeat = StudentCrowd::sample(index, tick * 0.1f);
            assert(glm::length(repeat.position - pose.position) == 0); // Render passes do not advance time.
        }
    }
    std::cout << "Student routes, walking speed, deterministic timing and vending clearance passed.\n";
}
