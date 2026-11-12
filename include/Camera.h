#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
    glm::vec3 position() const;
    void orbit(float deltaX, float deltaY);
    void zoom(float amount);

private:
    float yaw_{24.0f};
    float pitch_{9.0f};
    float distance_{9.8f};
};
