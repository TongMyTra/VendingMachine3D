#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    // The existing cabinet is 5.44 units tall: one world unit is 0.4 metres.
    static constexpr float metresPerUnit = 0.4f;
    static constexpr float eyeHeight = 1.65f / metresPerUnit - 0.10f;

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
    glm::vec3 position() const;
    bool canUseTrash() const;
    bool aimingAt(glm::vec3 point, float radius, float maxDistance) const;
    void toggleExternal() { external_ = !external_; }
    bool external() const { return external_; }
    bool presentationActive() const { return presentationActive_; }
    void updatePresentation(float dt, bool cinematic, bool vending, float progress, glm::vec3 product);
    void look(float deltaX, float deltaY);
    void setInteractionAspect(float aspect);
    void focusPayment(bool enabled);
    void setPickupMotion(float amount) { pickupMotion_ = amount; }
    void setDrinkMotion(float amount, float time) { drinkMotion_ = amount; drinkTime_ = time; }
    bool aimingAtPaymentSlot() const;
    void update(float deltaTime, float forward, float strafe, bool sprint);
    bool canInteract() const;
    bool beginInteraction();
    void endInteraction();
    bool interacting() const { return interacting_; }
    bool transitioning() const { return interacting_ && transitionTime_ < transitionDuration; }
    float interactionBlend() const;

private:
    bool external_{};
    bool presentationActive_{};
    glm::vec3 presentationEye_{};
    glm::vec3 presentationTarget_{};
    static constexpr float transitionDuration = 0.75f;
    glm::vec3 feetPosition_{0.0f, eyeHeight, 9.0f};
    float yaw_{-90.0f};
    float pitch_{-4.0f};
    float pickupMotion_{};
    float drinkMotion_{};
    float drinkTime_{};
    float bobPhase_{};
    float bobOffset_{};
    bool interacting_{};
    bool paymentFocus_{};
    float transitionTime_{};
    glm::vec3 transitionStart_{};
    float interactionAspect_{1.1f};
    float startYaw_{};
    float startPitch_{};
    glm::vec3 forwardDirection() const;
};
