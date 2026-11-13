#pragma once
#include <glm/vec3.hpp>

class Animation {
public:
    void start(int productIndex);
    void update(float deltaTime);
    bool active() const;
    int productIndex() const;
    float dropOffset() const;
    float progress() const;
    glm::vec3 deliveryPosition(glm::vec3 shelfPosition) const;
    float deliveryRotation() const;
    bool delivered() const { return delivered_; }
    void collect() { delivered_ = false; productIndex_ = -1; }
    void press(int index);
    bool pressing() const { return pressIndex_ >= 0; }
    int pressedIndex() const { return pressIndex_; }
    float pressProgress() const;
    float buttonDepth(int index) const;
    bool takeArrival();
    int takeSound(); // 1 button, 2 motor, 3 chute impact, 4 tray impact, 5 warning
    void warn() { sounds_ |= 16; }
private:
    float fallDuration() const;
    float duration() const;
    float elapsed_{};
    int productIndex_{-1};
    bool active_{};
    bool delivered_{};
    bool arrival_{};
    float pressElapsed_{};
    int pressIndex_{-1};
    int sounds_{};
};
