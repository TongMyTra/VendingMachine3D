#include "Renderer.h"
#include "Animation.h"
#include "Camera.h"
#include "CashInteraction.h"
#include "Payment.h"
#include "HeldDrink.h"
#include "SceneOptions.h"
#include "VendingMachine.h"
#include <OpenGL/OpenGL.h>
#include <dlfcn.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    CGLPixelFormatAttribute attributes[] = {kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0)};
    CGLPixelFormatObj format{};
    CGLContextObj context{};
    GLint count{};
    if (CGLChoosePixelFormat(attributes, &format, &count) != kCGLNoError || !format) return 77;
    const auto status = CGLCreateContext(format, nullptr, &context);
    CGLDestroyPixelFormat(format);
    if (status != kCGLNoError || !context) return 77;
    CGLSetCurrentContext(context);
    if (!gladLoadGL([](const char* name) { return reinterpret_cast<GLADapiproc>(dlsym(RTLD_DEFAULT, name)); })) return 1;
    int result = 0;
    try {
        GLuint fbo{}, color{}, depth{};
        glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glGenTextures(1, &color); glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1100, 1000, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
        glGenRenderbuffers(1, &depth); glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1100, 1000);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) throw std::runtime_error("Incomplete FBO");
        Renderer renderer(VM3D_SHADER_DIR);
        VendingMachine machine; Animation animation; Camera camera;
        CashInteraction cash; Payment payment; HeldDrink drink; SceneOptions options;
        std::array<std::uint64_t, 3> hashes{};
        for (int mode = 0; mode < 3; ++mode) {
            options.wireframe = mode == 1;
            options.xray = mode == 2;
            renderer.render(machine, animation, camera, cash, payment, drink, options, 1100, 1000);
            GLint restoredFbo = 0;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &restoredFbo);
            if (static_cast<GLuint>(restoredFbo) != fbo) throw std::runtime_error("Reflection pass leaked its framebuffer");
            if (glGetError() != GL_NO_ERROR) throw std::runtime_error("OpenGL render error");
            GLint polygon[2]{}; glGetIntegerv(GL_POLYGON_MODE, polygon);
            if (polygon[0] != GL_FILL) throw std::runtime_error("Wireframe leaked into UI pass");
            std::vector<unsigned char> pixels(1100 * 1000 * 4);
            glReadPixels(0, 0, 1100, 1000, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            for (auto value : pixels) hashes[mode] = hashes[mode] * 131 + value;
            if (argc > 1 && mode == 0) {
                std::ofstream output(argv[1], std::ios::binary);
                output << "P6\n1100 1000\n255\n";
                for (int y = 999; y >= 0; --y) for (int x = 0; x < 1100; ++x)
                    output.write(reinterpret_cast<const char*>(&pixels[(y * 1100 + x) * 4]), 3);
            }
        }
        if (hashes[0] == hashes[1] || hashes[0] == hashes[2]) throw std::runtime_error("Render modes produced identical frames");
        // Exercise transformed product surfaces and camera transitions on a real GL context.
        options = {};
        drink.pickUp(machine.products()[0], {-0.43f, 0.49f, 1.17f});
        drink.update(1, false);
        drink.open(); drink.update(0.8f, false);
        renderer.render(machine, animation, camera, cash, payment, drink, options, 1100, 1000);
        drink.update(1, false);
        drink.toggleInspect(); drink.inspectRotate(190, -30); drink.inspectZoom(2); drink.update(0.5f, false);
        renderer.render(machine, animation, camera, cash, payment, drink, options, 1100, 1000);
        drink.toggleInspect(); drink.update(1, false);
        drink.putDown({4.8f, 2.49f, 0.8f}); drink.update(1, false);
        renderer.render(machine, animation, camera, cash, payment, drink, options, 1100, 1000);
        camera.toggleExternal();
        for (int i = 0; i < 120; ++i) camera.updatePresentation(0.01f, true, false, 0, {});
        renderer.render(machine, animation, camera, cash, payment, drink, options, 1100, 1000);
        if (glGetError() != GL_NO_ERROR) throw std::runtime_error("Inspection/placement/external-camera render error");
        // Resize the target to a desktop POV and exercise reflection-target reallocation.
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1600, 960, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1600, 960);
        Camera lobbyCamera(true);
        renderer.render(machine, animation, lobbyCamera, cash, payment, drink, options, 1600, 960);
        if (glGetError() != GL_NO_ERROR) throw std::runtime_error("Reflection resize render error");
        if (argc > 2) {
            std::vector<unsigned char> pixels(1600 * 960 * 4);
            glReadPixels(0, 0, 1600, 960, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            std::ofstream output(argv[2], std::ios::binary);
            output << "P6\n1600 960\n255\n";
            for (int y = 959; y >= 0; --y) for (int x = 0; x < 1600; ++x)
                output.write(reinterpret_cast<const char*>(&pixels[(y * 1600 + x) * 4]), 3);
        }
        glDeleteRenderbuffers(1, &depth); glDeleteTextures(1, &color); glDeleteFramebuffers(1, &fbo);
        std::cout << "Shaders, textures, solid/wireframe/X-Ray rendering and polygon-state restoration passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; result = 1;
    }
    CGLSetCurrentContext(nullptr); CGLDestroyContext(context);
    return result;
}
