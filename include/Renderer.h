#pragma once

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <string>

class Animation;
class Camera;
class CashInteraction;
class Payment;
class HeldDrink;
struct SceneOptions;
struct Product;
class VendingMachine;

class Renderer {
public:
    Renderer(const std::string& shaderDirectory);
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void render(const VendingMachine& machine, const Animation& animation,
                const Camera& camera, const CashInteraction& cash, const Payment& payment, const HeldDrink& drink, const SceneOptions& options, int width, int height);

private:
    void createProductTextures();
    void drawEnvironment(const glm::mat4& vp);
    void ensureReflectionTarget(int width, int height);
    GLuint reflectionFbo_{};
    GLuint reflectionTexture_{};
    GLuint reflectionDepth_{};
    int reflectionWidth_{};
    int reflectionHeight_{};
    bool reflectionPass_{};
    bool floorMaterial_{};
    GLuint productTextures_[12]{};
    bool useTexture_{};
    GLuint activeTexture_{};
    bool suppressGeometry_{};
    bool wireframe_{};
    glm::mat4 parentTransform_{1.0f};
    void drawHeldDrink(const glm::mat4& vp, const Camera& camera, const HeldDrink& drink);
    void drawPressHand(const glm::mat4& vp, const Camera& camera, const Animation& animation);
    void drawCash(const glm::mat4& vp, const Camera& camera, const CashInteraction& cash);
    GLuint program_{};
    GLuint vao_{};
    GLuint vbo_{};
    GLint floorLocation_{};
    GLint reflectionPassLocation_{};
    GLint wireframeLocation_{};
    GLint textureEnabledLocation_{};
    GLint cameraLocation_{};
    GLint reflectionSamplerLocation_{};
    GLint viewportLocation_{};
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
                     const glm::vec3& position, bool selected, float opening = 0.0f);
    void drawMesh(const glm::mat4& viewProjection, const glm::vec3& position,
                  const glm::vec3& scale, const glm::vec3& color, GLint first,
                  GLint count, float opacity = 1.0f, float emissive = 0.0f);
    void drawText(const glm::mat4& vp, const std::string& text,
                  glm::vec3 center, float height, glm::vec3 color);
    GLuint compileShader(GLenum type, const std::string& path);
    static std::string readFile(const std::string& path);
};
