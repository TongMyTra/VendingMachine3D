#include "Renderer.h"

#include "Animation.h"
#include "Camera.h"
#include "VendingMachine.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Face {
    std::array<int, 4> corners;
    glm::vec3 normal;
};

constexpr std::array<glm::vec3, 8> cubeCorners{{
    {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
    {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
    {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
    {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
}};

constexpr std::array<Face, 6> cubeFaces{{
    {{{4, 5, 6, 7}}, {0.0f, 0.0f, 1.0f}},
    {{{1, 0, 3, 2}}, {0.0f, 0.0f, -1.0f}},
    {{{0, 4, 7, 3}}, {-1.0f, 0.0f, 0.0f}},
    {{{5, 1, 2, 6}}, {1.0f, 0.0f, 0.0f}},
    {{{3, 7, 6, 2}}, {0.0f, 1.0f, 0.0f}},
    {{{0, 1, 5, 4}}, {0.0f, -1.0f, 0.0f}},
}};

void appendVertex(std::vector<float>& data, const glm::vec3& position,
                  const glm::vec3& normal) {
    data.insert(data.end(), {position.x, position.y, position.z,
                             normal.x, normal.y, normal.z});
}

std::pair<GLint, GLint> appendCylinder(std::vector<float>& data, float bottomRadius,
                                       float topRadius, int segments) {
    const GLint first = static_cast<GLint>(data.size() / 6);
    for (int i = 0; i < segments; ++i) {
        const float a0 = glm::two_pi<float>() * static_cast<float>(i) /
                         static_cast<float>(segments);
        const float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) /
                         static_cast<float>(segments);
        const glm::vec3 bottom0{bottomRadius * std::cos(a0), -0.5f,
                                bottomRadius * std::sin(a0)};
        const glm::vec3 bottom1{bottomRadius * std::cos(a1), -0.5f,
                                bottomRadius * std::sin(a1)};
        const glm::vec3 top0{topRadius * std::cos(a0), 0.5f,
                             topRadius * std::sin(a0)};
        const glm::vec3 top1{topRadius * std::cos(a1), 0.5f,
                             topRadius * std::sin(a1)};
        const glm::vec3 normal0 =
            glm::normalize(glm::vec3(std::cos(a0), bottomRadius - topRadius,
                                     std::sin(a0)));
        const glm::vec3 normal1 =
            glm::normalize(glm::vec3(std::cos(a1), bottomRadius - topRadius,
                                     std::sin(a1)));
        appendVertex(data, bottom0, normal0);
        appendVertex(data, top0, normal0);
        appendVertex(data, top1, normal1);
        appendVertex(data, bottom0, normal0);
        appendVertex(data, top1, normal1);
        appendVertex(data, bottom1, normal1);

        appendVertex(data, {0.0f, -0.5f, 0.0f}, {0.0f, -1.0f, 0.0f});
        appendVertex(data, bottom1, {0.0f, -1.0f, 0.0f});
        appendVertex(data, bottom0, {0.0f, -1.0f, 0.0f});
        appendVertex(data, {0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f});
        appendVertex(data, top0, {0.0f, 1.0f, 0.0f});
        appendVertex(data, top1, {0.0f, 1.0f, 0.0f});
    }
    return {first, static_cast<GLint>(data.size() / 6) - first};
}
}

Renderer::Renderer(const std::string& shaderDirectory) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, shaderDirectory + "/vertex.glsl");
    const GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, shaderDirectory + "/fragment.glsl");
    program_ = glCreateProgram();
    glAttachShader(program_, vertexShader);
    glAttachShader(program_, fragmentShader);
    glLinkProgram(program_);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(program_, logLength, nullptr, log.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error("Shader program link failed: " + log);
    }

    mvpLocation_ = glGetUniformLocation(program_, "uMVP");
    modelLocation_ = glGetUniformLocation(program_, "uModel");
    colorLocation_ = glGetUniformLocation(program_, "uColor");
    lightLocation_ = glGetUniformLocation(program_, "uLightPosition");
    opacityLocation_ = glGetUniformLocation(program_, "uOpacity");
    emissiveLocation_ = glGetUniformLocation(program_, "uEmissive");

    std::vector<float> vertices;
    constexpr int triangleCorners[6] = {0, 1, 2, 0, 2, 3};
    vertices.reserve(36 * 6);
    for (const Face& face : cubeFaces) {
        for (const int corner : triangleCorners) {
            appendVertex(vertices, cubeCorners[static_cast<std::size_t>(
                                       face.corners[static_cast<std::size_t>(corner)])],
                         face.normal);
        }
    }
    const auto cylinderRange = appendCylinder(vertices, 0.5f, 0.5f, 32);
    cylinderFirst_ = cylinderRange.first;
    cylinderCount_ = cylinderRange.second;
    const auto frustumRange = appendCylinder(vertices, 0.5f, 0.24f, 32);
    frustumFirst_ = frustumRange.first;
    frustumCount_ = frustumRange.second;

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

Renderer::~Renderer() {
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
    }
}

std::string Renderer::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open shader file: " + path);
    }
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

GLuint Renderer::compileShader(GLenum type, const std::string& path) {
    const std::string source = readFile(path);
    const char* sourcePointer = source.c_str();
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile failed (" + path + "): " + log);
    }
    return shader;
}

void Renderer::drawMesh(const glm::mat4& viewProjection, const glm::vec3& position,
                        const glm::vec3& scale, const glm::vec3& color, GLint first,
                        GLint count, float opacity, float emissive) {
    const glm::mat4 model = glm::translate(glm::mat4(1.0f), position) *
                            glm::scale(glm::mat4(1.0f), scale);
    const glm::mat4 mvp = viewProjection * model;
    glUniformMatrix4fv(modelLocation_, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(mvpLocation_, 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(colorLocation_, 1, &color[0]);
    glUniform1f(opacityLocation_, opacity);
    glUniform1f(emissiveLocation_, emissive);
    glDrawArrays(GL_TRIANGLES, first, count);
}

void Renderer::drawBox(const glm::mat4& viewProjection, const glm::vec3& position,
                       const glm::vec3& scale, const glm::vec3& color, float emissive) {
    drawMesh(viewProjection, position, scale, color, 0, 36, 1.0f, emissive);
}

void Renderer::drawCylinder(const glm::mat4& viewProjection, const glm::vec3& position,
                            const glm::vec3& scale, const glm::vec3& color,
                            bool tapered) {
    drawMesh(viewProjection, position, scale, color,
             tapered ? frustumFirst_ : cylinderFirst_,
             tapered ? frustumCount_ : cylinderCount_);
}

void Renderer::drawProduct(const glm::mat4& viewProjection, const Product& product,
                           const glm::vec3& position, bool selected) {
    const float brightness = selected ? 1.16f : 1.0f;
    const glm::vec3 color = glm::min(product.color * brightness, glm::vec3(1.0f));
    switch (product.type) {
    case ProductType::Can: {
        drawCylinder(viewProjection, position, {0.34f, 0.58f, 0.34f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.294f, position.z},
                     {0.34f, 0.025f, 0.34f}, {0.66f, 0.70f, 0.70f});
        drawCylinder(viewProjection, {position.x, position.y - 0.294f, position.z},
                     {0.34f, 0.018f, 0.34f}, {0.56f, 0.59f, 0.59f});
        if (product.id == 1 || product.id == 3) {
            drawCylinder(viewProjection, {position.x, position.y + 0.02f,
                                           position.z},
                         {0.348f, 0.20f, 0.348f}, {0.93f, 0.91f, 0.84f});
            drawCylinder(viewProjection, {position.x, position.y - 0.105f,
                                           position.z},
                         {0.35f, 0.045f, 0.35f}, color);
        } else {
            drawCylinder(viewProjection, {position.x, position.y + 0.03f,
                                           position.z},
                         {0.348f, 0.16f, 0.348f}, {0.93f, 0.92f, 0.87f});
            drawCylinder(viewProjection, {position.x, position.y - 0.105f,
                                           position.z},
                         {0.35f, 0.055f, 0.35f}, {0.84f, 0.045f, 0.055f});
        }
        break;
    }
    case ProductType::Bottle: {
        drawCylinder(viewProjection, {position.x, position.y - 0.045f, position.z},
                     {0.30f, 0.43f, 0.30f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.245f, position.z},
                     {0.30f, 0.18f, 0.30f}, color, true);
        drawCylinder(viewProjection, {position.x, position.y + 0.36f, position.z},
                     {0.13f, 0.12f, 0.13f}, color);
        drawCylinder(viewProjection, {position.x, position.y + 0.43f, position.z},
                     {0.15f, 0.055f, 0.15f},
                     product.id == 7 ? glm::vec3(0.90f, 0.25f, 0.05f)
                                     : glm::vec3(0.12f, 0.32f, 0.72f));
        drawCylinder(viewProjection, {position.x, position.y - 0.055f, position.z},
                     {0.307f, 0.15f, 0.307f}, {0.92f, 0.92f, 0.84f});
        break;
    }
    case ProductType::Snack:
        drawBox(viewProjection, position, {0.40f, 0.53f, 0.28f}, color);
        drawBox(viewProjection, {position.x, position.y + 0.25f, position.z},
                {0.39f, 0.055f, 0.29f},
                glm::min(color * 0.74f, glm::vec3(1.0f)));
        drawBox(viewProjection, {position.x, position.y + 0.015f,
                                 position.z + 0.145f},
                {0.31f, 0.13f, 0.014f}, {0.96f, 0.88f, 0.67f});
        break;
    case ProductType::Bakery:
        drawBox(viewProjection, position, {0.43f, 0.48f, 0.30f}, color);
        drawBox(viewProjection, {position.x, position.y, position.z + 0.156f},
                {0.34f, 0.17f, 0.016f}, {0.94f, 0.87f, 0.70f});
        drawBox(viewProjection, {position.x, position.y + 0.225f, position.z},
                {0.44f, 0.035f, 0.31f},
                glm::min(color * 0.72f, glm::vec3(1.0f)));
        break;
    }
    if (selected) {
        drawBox(viewProjection, {position.x, position.y - 0.36f, position.z + 0.02f},
                {0.48f, 0.018f, 0.42f}, {0.20f, 0.96f, 0.86f}, 1.1f);
    }
}

void Renderer::render(const VendingMachine& machine, const Animation& animation,
                      const Camera& camera, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.025f, 0.035f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 viewProjection =
        camera.projectionMatrix(static_cast<float>(width) / static_cast<float>(height)) *
        camera.viewMatrix();
    glUseProgram(program_);
    glUniform3f(lightLocation_, -3.0f, 7.0f, 4.0f);
    glBindVertexArray(vao_);

    drawBox(viewProjection, {0.0f, -0.12f, 0.0f}, {30.0f, 0.2f, 30.0f},
            {0.12f, 0.14f, 0.16f});

    // Brushed-metal cabinet shell and the dark interior behind the product window.
    drawBox(viewProjection, {0.0f, 2.65f, -0.72f}, {4.65f, 5.3f, 0.12f},
            {0.21f, 0.24f, 0.27f});
    drawBox(viewProjection, {-2.26f, 2.65f, -0.02f}, {0.13f, 5.3f, 1.42f},
            {0.30f, 0.33f, 0.36f});
    drawBox(viewProjection, {2.26f, 2.65f, -0.02f}, {0.13f, 5.3f, 1.42f},
            {0.30f, 0.33f, 0.36f});
    drawBox(viewProjection, {0.0f, 5.27f, -0.02f}, {4.65f, 0.12f, 1.42f},
            {0.35f, 0.38f, 0.40f});
    drawBox(viewProjection, {0.0f, 0.03f, -0.02f}, {4.65f, 0.12f, 1.42f},
            {0.32f, 0.35f, 0.37f});
    drawBox(viewProjection, {-0.48f, 5.27f, 0.04f}, {3.52f, 0.09f, 1.40f},
            {0.42f, 0.45f, 0.47f});
    drawBox(viewProjection, {-0.48f, 0.03f, 0.04f}, {3.52f, 0.10f, 1.40f},
            {0.38f, 0.41f, 0.43f});
    drawBox(viewProjection, {-2.19f, 2.65f, 0.04f}, {0.10f, 5.12f, 1.40f},
            {0.40f, 0.43f, 0.45f});
    drawBox(viewProjection, {1.23f, 2.65f, 0.04f}, {0.10f, 5.12f, 1.40f},
            {0.40f, 0.43f, 0.45f});

    for (int shelf = 0; shelf < 5; ++shelf) {
        const float productY = 4.56f - static_cast<float>(shelf) * 0.86f;
        const float shelfY = productY - 0.38f;
        drawBox(viewProjection, {-0.48f, shelfY, 0.39f}, {3.25f, 0.07f, 0.88f},
                {0.45f, 0.49f, 0.51f});
        drawBox(viewProjection, {-0.48f, shelfY + 0.05f, 0.84f},
                {3.12f, 0.035f, 0.045f}, {0.66f, 0.82f, 0.88f}, 0.22f);
        drawBox(viewProjection, {-2.03f, productY + 0.02f, 0.34f},
                {0.035f, 0.52f, 0.07f}, {0.38f, 0.47f, 0.50f});
    }

    // Two vertical LED rails illuminate the product rows.
    drawBox(viewProjection, {-2.02f, 2.72f, 0.61f}, {0.035f, 4.76f, 0.035f},
            {0.42f, 0.83f, 0.96f}, 0.85f);
    drawBox(viewProjection, {1.05f, 2.72f, 0.61f}, {0.035f, 4.76f, 0.035f},
            {0.42f, 0.83f, 0.96f}, 0.85f);

    for (int i = 0; i < static_cast<int>(machine.products().size()); ++i) {
        const Product& product = machine.products()[static_cast<std::size_t>(i)];
        glm::vec3 position = machine.productPosition(i);
        if (animation.active() && animation.productIndex() == i) {
            const float progress = animation.progress();
            position.x = glm::mix(position.x, -0.48f, progress);
            position.y = glm::mix(position.y, 0.48f, progress);
            position.z = 0.48f;
            drawProduct(viewProjection, product, position, false);
        } else if (product.stock > 0) {
            drawProduct(viewProjection, product, position, i == machine.selectedIndex());
        }
    }

    // Right-side payment controls, LCD, numeric keypad and cash/change slots.
    drawBox(viewProjection, {1.68f, 2.66f, 0.715f}, {0.84f, 4.84f, 0.12f},
            {0.15f, 0.17f, 0.19f});
    drawBox(viewProjection, {1.68f, 4.55f, 0.79f}, {0.66f, 0.72f, 0.055f},
            {0.025f, 0.11f, 0.13f});
    drawBox(viewProjection, {1.68f, 4.55f, 0.825f}, {0.55f, 0.04f, 0.012f},
            {0.19f, 0.94f, 0.75f}, 1.0f);
    drawBox(viewProjection, {1.68f, 4.40f, 0.825f}, {0.42f, 0.025f, 0.012f},
            {0.14f, 0.69f, 0.59f}, 0.75f);
    drawBox(viewProjection, {1.68f, 4.26f, 0.825f}, {0.48f, 0.025f, 0.012f},
            {0.14f, 0.69f, 0.59f}, 0.75f);

    for (int key = 0; key < 12; ++key) {
        const int row = key / 3;
        const int column = key % 3;
        const float x = 1.48f + static_cast<float>(column) * 0.20f;
        const float y = 3.63f - static_cast<float>(row) * 0.20f;
        drawBox(viewProjection, {x, y, 0.805f}, {0.13f, 0.12f, 0.07f},
                {0.45f, 0.48f, 0.50f});
        drawBox(viewProjection, {x, y, 0.846f}, {0.075f, 0.065f, 0.012f},
                {0.72f, 0.76f, 0.77f});
    }

    drawBox(viewProjection, {1.68f, 2.57f, 0.805f}, {0.48f, 0.055f, 0.075f},
            {0.035f, 0.045f, 0.05f});
    drawBox(viewProjection, {1.68f, 2.72f, 0.85f}, {0.50f, 0.035f, 0.02f},
            {0.77f, 0.79f, 0.80f});
    drawBox(viewProjection, {1.68f, 1.91f, 0.805f}, {0.48f, 0.12f, 0.08f},
            {0.045f, 0.055f, 0.06f});
    drawBox(viewProjection, {1.68f, 1.91f, 0.85f}, {0.32f, 0.025f, 0.018f},
            {0.60f, 0.64f, 0.66f});
    drawBox(viewProjection, {1.68f, 1.47f, 0.805f}, {0.48f, 0.30f, 0.10f},
            {0.035f, 0.045f, 0.05f});
    drawBox(viewProjection, {1.68f, 1.52f, 0.86f}, {0.32f, 0.045f, 0.018f},
            {0.18f, 0.92f, 0.69f}, 0.65f);

    // Recessed pickup bay and its illuminated lip.
    drawBox(viewProjection, {-0.48f, 0.49f, 0.60f}, {1.62f, 0.55f, 0.76f},
            {0.065f, 0.075f, 0.08f});
    drawBox(viewProjection, {-0.48f, 0.76f, 1.00f}, {1.26f, 0.045f, 0.055f},
            {0.47f, 0.86f, 0.95f}, 0.9f);

    // Slightly tinted transparent glass sits in front of products and shelves.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthMask(GL_FALSE);
    drawMesh(viewProjection, {-0.48f, 2.88f, 0.70f}, {3.17f, 4.20f, 0.018f},
             {0.35f, 0.75f, 0.88f}, 0, 36, 0.12f);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    // Bright corner indicators make the cabinet lighting visible in a static scene.
    for (int side : {-1, 1}) {
        for (int row = 0; row < 5; ++row) {
            drawBox(viewProjection,
                    {static_cast<float>(side) * 2.18f,
                     4.65f - static_cast<float>(row) * 0.86f, 0.75f},
                    {0.055f, 0.07f, 0.04f}, {0.28f, 0.83f, 1.0f}, 1.0f);
        }
    }

    glBindVertexArray(0);
    glUseProgram(0);
}
