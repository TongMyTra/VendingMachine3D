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
    float yaw_{0.0f};
    float pitch_{8.0f};
    float distance_{7.5f};
};
