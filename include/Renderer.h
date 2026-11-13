#pragma once

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <string>

class Animation;
class Camera;
class CashInteraction;
class Payment;
struct Product;
class VendingMachine;

class Renderer {
public:
    Renderer(const std::string& shaderDirectory);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render(const VendingMachine& machine, const Animation& animation,
                const Camera& camera, const CashInteraction& cash, const Payment& payment, int width, int height);

private:
    glm::mat4 parentTransform_{1.0f};
    void drawCash(const glm::mat4& vp, const Camera& camera, const CashInteraction& cash);
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLint mvpLocation_{};
    GLint modelLocation_{};
    GLint colorLocation_{};
    GLint lightLocation_{};
    GLint opacityLocation_{};
    GLint emissiveLocation_{};
    GLint roundedFirst_{};
    GLint roundedCount_{};
    GLint cylinderFirst_{};
    GLint cylinderCount_{};
    GLint frustumFirst_{};
    GLint frustumCount_{};
    GLint coneFirst_{};
    GLint coneCount_{};

    void drawBox(const glm::mat4& viewProjection, const glm::vec3& position,
                 const glm::vec3& scale, const glm::vec3& color,
                 float emissive = 0.0f);
    void drawCylinder(const glm::mat4& viewProjection, const glm::vec3& position,
                      const glm::vec3& scale, const glm::vec3& color,
                      bool tapered = false);
    void drawProduct(const glm::mat4& viewProjection, const Product& product,
                     const glm::vec3& position, bool selected);
    void drawMesh(const glm::mat4& viewProjection, const glm::vec3& position,
                  const glm::vec3& scale, const glm::vec3& color, GLint first,
                  GLint count, float opacity = 1.0f, float emissive = 0.0f);
    void drawText(const glm::mat4& vp, const std::string& text,
                  glm::vec3 center, float height, glm::vec3 color);
    GLuint compileShader(GLenum type, const std::string& path);
    static std::string readFile(const std::string& path);
};
