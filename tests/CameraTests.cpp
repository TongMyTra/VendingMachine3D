#include "Camera.h"
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
void step(Camera& camera, int frames, float forward, float strafe = 0.0f, bool sprint = false) {
    for (int i = 0; i < frames; ++i) camera.update(0.01f, forward, strafe, sprint);
}
float horizontalDistance(glm::vec3 a, glm::vec3 b) {
    a.y = b.y = 0.0f;
    return glm::length(a - b);
}
}
int main() {
    Camera straight, diagonal, fast;
    const auto initial = straight.position();
    assert(std::abs((initial.y + 0.10f) * Camera::metresPerUnit - 1.65f) < 0.0001f);
    assert(!straight.beginInteraction()); // Too far away.
    step(straight, 50, 1.0f);
    step(diagonal, 50, 1.0f, 1.0f);
    step(fast, 50, 1.0f, 0.0f, true);
    const float walking = horizontalDistance(initial, straight.position());
    assert(std::abs(walking - horizontalDistance(initial, diagonal.position())) < 0.001f);
    assert(std::abs(2.0f * walking - horizontalDistance(initial, fast.position())) < 0.001f);
    assert(fast.canInteract());
    fast.look(1800.0f, 0.0f);
    assert(!fast.canInteract()); // Looking away.
    fast.look(-1800.0f, 0.0f);
    assert(fast.beginInteraction());
    const auto start = fast.position();
    fast.update(0.01f, 0.0f, 0.0f, false);
    assert(horizontalDistance(start, fast.position()) < 0.01f); // No teleport.
    step(fast, 100, 1.0f, 1.0f, true);
    assert(!fast.transitioning());
    assert(glm::length(fast.position() - glm::vec3(0.0f, Camera::eyeHeight, 6.8f)) < 0.001f);
    // Entire cabinet stays inside the scene viewport at common and narrow aspect ratios.
    for (float aspect : {1.1f, 0.66f, 0.4f, 2.0f}) {
        fast.setInteractionAspect(aspect);
        step(fast, 1, 0.0f);
        const auto vp = fast.projectionMatrix(aspect) * fast.viewMatrix();
        for (float x : {-2.35f, 2.35f})
            for (float y : {-0.10f, 5.44f})
                for (float z : {-0.80f, 1.05f}) {
                    const auto clip = vp * glm::vec4(x, y, z, 1.0f);
                    assert(clip.w > 0.0f);
                    assert(std::abs(clip.x / clip.w) < 0.95f);
                    assert(std::abs(clip.y / clip.w) < 0.95f);
                }
    }
    fast.setInteractionAspect(1.1f);
    step(fast, 1, 0.0f);
    fast.focusPayment(true);
    assert(fast.transitioning());
    assert(!fast.aimingAtPaymentSlot());
    step(fast, 100, 0.0f);
    assert(fast.aimingAtPaymentSlot());
    fast.look(900.0f, 0.0f);
    assert(!fast.aimingAtPaymentSlot());
    fast.look(-900.0f, 0.0f);
    assert(fast.aimingAtPaymentSlot());
    fast.focusPayment(false);
    step(fast, 100, 0.0f);
    const auto dock = fast.position();
    fast.look(1000.0f, 1000.0f);
    step(fast, 10, 1.0f);
    assert(glm::length(dock - fast.position()) < 0.001f); // Locked during interaction.
    fast.endInteraction();
    assert(!fast.interacting());
    assert(glm::length(dock - fast.position()) < 0.001f);
    assert(fast.beginInteraction());
    step(fast, 10, 0.0f);
    const auto partial = fast.position();
    fast.endInteraction();
    assert(glm::length(partial - fast.position()) < 0.001f); // Cancel keeps current pose.
    Camera collision;
    step(collision, 1000, 1.0f, 0.0f, true);
    assert(collision.position().z >= 1.35f); // Cannot walk through cabinet.
    step(collision, 100, 0.0f);
    assert(std::abs(collision.position().y - Camera::eyeHeight) < 0.001f);
    collision.look(0.0f, 100000.0f);
    const auto view = collision.viewMatrix();
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j) assert(std::isfinite(view[i][j]));
    Camera modes;
    const auto original = modes.position();
    modes.toggleExternal();
    modes.updatePresentation(0.01f, true, false, 0, {});
    assert(modes.external() && modes.presentationActive());
    assert(glm::length(modes.position() - original) < 1.0f); // Smooth entry.
    for (int i = 0; i < 200; ++i) modes.updatePresentation(0.01f, true, false, 0, {});
    assert(glm::length(modes.position() - glm::vec3(7, 5.4f, 10.5f)) < 0.01f);
    modes.toggleExternal();
    for (int i = 0; i < 300; ++i) modes.updatePresentation(0.01f, true, false, 0, {});
    assert(!modes.presentationActive());
    assert(glm::length(modes.position() - original) < 0.001f);
    modes.updatePresentation(0.01f, true, true, 0.1f, {0, 4.33f, 0.5f});
    assert(modes.presentationActive() && !modes.canInteract());
    for (int i = 0; i < 100; ++i) modes.updatePresentation(0.01f, true, true, 0.7f, {-0.4f, 1.2f, 1});
    for (int i = 0; i < 300; ++i) modes.updatePresentation(0.01f, true, false, 1, {-0.43f, 0.49f, 1.17f});
    assert(!modes.presentationActive());
    assert(glm::length(modes.position() - original) < 0.001f); // Saved POV pose survives playback.
    Camera trash;
    assert(!trash.canUseTrash());
    step(trash, 140, 0, -1);
    step(trash, 180, 1);
    trash.look(0, 460); // Aim down at the front body, below the old hidden target.
    assert(trash.canUseTrash());
    trash.look(1800, 0);
    assert(!trash.canUseTrash());
    std::cout << "Camera movement, sprint, collision, interaction and transition checks passed.\n";
}
