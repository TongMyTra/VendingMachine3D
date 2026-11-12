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

    const glm::mat4 viewProjection =
        camera.projectionMatrix(static_cast<float>(width) / static_cast<float>(height)) *
        camera.viewMatrix();
    float closestDistance = 0.055f;
    int closestIndex = -1;

    for (int i = 0; i < static_cast<int>(machine_.products().size()); ++i) {
        const glm::vec3 position = machine_.productPosition(i);
        const glm::vec4 clip = viewProjection * glm::vec4(position, 1.0f);
        if (clip.w <= 0.0f) {
            continue;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        const float screenX = (ndc.x + 1.0f) * 0.5f * static_cast<float>(width);
        const float screenY = (1.0f - ndc.y) * 0.5f * static_cast<float>(height);
        const float dx = (screenX - cursorX) / static_cast<float>(width);
        const float dy = (screenY - cursorY) / static_cast<float>(height);
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance < closestDistance) {
            closestDistance = distance;
            closestIndex = i;
        }
    }

    if (closestIndex >= 0) {
        machine_.select(closestIndex);
        machine_.setStatus("Đã chọn sản phẩm. Hãy nạp tiền và bấm Mua hàng.");
    }
}

void Interaction::selectNext(int direction) {
    const int count = static_cast<int>(machine_.products().size());
    if (count > 0) {
        machine_.select((machine_.selectedIndex() + direction + count) % count);
        machine_.setStatus("Đã chọn sản phẩm. Hãy nạp tiền và bấm Mua hàng.");
    }
}

bool Interaction::purchase() {
    return machine_.vendSelected(payment_, animation_);
}
