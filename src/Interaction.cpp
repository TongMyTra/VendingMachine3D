#include "Interaction.h"

#include "Animation.h"
#include "Camera.h"
#include "Payment.h"
#include "VendingMachine.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Interaction::Interaction(VendingMachine& machine, Payment& payment, Animation& animation)
    : machine_(machine), payment_(payment), animation_(animation) {}

void Interaction::click(float cursorX, float cursorY, int width, int height,
                        const Camera& camera) {
    if (width <= 0 || height <= 0) {
        return;
    }

    if (animation_.active() || animation_.pressing()) return;
    const auto vp = camera.projectionMatrix(static_cast<float>(width) / height) * camera.viewMatrix();
    // Cast the cursor ray onto the physical keypad plane.
    const glm::vec4 nearClip = glm::inverse(vp) * glm::vec4(2 * cursorX / width - 1, 1 - 2 * cursorY / height, -1, 1);
    const glm::vec3 ray = glm::normalize(glm::vec3(nearClip) / nearClip.w - camera.position());
    if (ray.z >= -0.001f) return;
    const float distance = (0.78f - camera.position().z) / ray.z;
    if (distance <= 0) return;
    const auto hit = camera.position() + ray * distance;
    for (int i = 0; i < 12; ++i) {
        const float x = 1.46f + (i % 3) * 0.22f;
        const float y = 3.70f - (i / 3) * 0.22f;
        if (std::abs(hit.x - x) < 0.09f && std::abs(hit.y - y) < 0.09f) { select(i); return; }
    }
}

void Interaction::hover(float x, float y, int width, int height, const Camera& camera) {
    machine_.hover(-1);
    if (width <= 0 || height <= 0) return;
    const auto vp = camera.projectionMatrix(static_cast<float>(width) / height) * camera.viewMatrix();
    const auto clip = glm::inverse(vp) * glm::vec4(2 * x / width - 1, 1 - 2 * y / height, -1, 1);
    const auto ray = glm::normalize(glm::vec3(clip) / clip.w - camera.position());
    if (ray.z >= -0.001f || camera.position().z < 1.25f) return;
    const float t = (0.43f - camera.position().z) / ray.z;
    if (t < 0 || t > 15) return;
    const auto hit = camera.position() + ray * t;
    for (int i = 0; i < static_cast<int>(machine_.products().size()); ++i) {
        const auto p = machine_.productPosition(i);
        if (std::abs(hit.x - p.x) < 0.25f && std::abs(hit.y - p.y) < 0.40f) {
            machine_.hover(i); return;
        }
    }
}

void Interaction::select(int index) {
    if (animation_.active() || animation_.pressing()) return;
    if (index < 0 || index >= static_cast<int>(machine_.products().size())) return;
    machine_.select(index);
    if (machine_.selectedProduct()->stock <= 0) {
        machine_.setStatus("Sản phẩm đã hết. Hãy chọn món khác.");
        animation_.warn();
        return;
    }
    animation_.press(index);
    machine_.setStatus(machine_.selectedProduct()->stock > 0 ?
        "Đã chọn sản phẩm. Xác nhận MUA HÀNG để thanh toán." : "Sản phẩm đã hết. Hãy chọn món khác.");
}

void Interaction::selectNext(int direction) {
    const int count = static_cast<int>(machine_.products().size());
    if (count > 0) {
        select((machine_.selectedIndex() + direction + count) % count);
    }
}

bool Interaction::purchase() {
    const bool success = machine_.vendSelected(payment_, animation_);
    if (!success) animation_.warn();
    return success;
}
