#include "Animation.h"
#include "Camera.h"
#include "CashInteraction.h"
#include "HeldDrink.h"
#include "SceneOptions.h"
#include "Sound.h"
#include "Interaction.h"
#include "Payment.h"
#include "Renderer.h"
#include "VendingMachine.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr float sceneRatio = 1.0f;

struct AppState {
    Camera* camera{};
    Interaction* interaction{};
    CashInteraction* cash{};
    Payment* payment{};
    Animation* animation{};
    HeldDrink* drink{};
    SceneOptions* options{};
    VendingMachine* machine{};
    bool exitRequested{};
    bool mouseCaptured{true};
    float viewportRatio{1.0f};
    bool hasPreviousCursor{};
    double previousX{};
    double previousY{};
};

AppState* appState(GLFWwindow* window) {
    return static_cast<AppState*>(glfwGetWindowUserPointer(window));
}

void captureMouse(GLFWwindow* window, AppState& state, bool capture) {
    state.mouseCaptured = capture;
    state.hasPreviousCursor = false;
    glfwSetInputMode(window, GLFW_CURSOR, capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (capture) ImGui::SetWindowFocus(nullptr);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    AppState* state = appState(window);
    if (!state || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
    if (state->drink->inspecting() || state->camera->presentationActive()) return;
    if (!state->camera->interacting()) {
        if (!state->mouseCaptured && !ImGui::GetIO().WantCaptureMouse)
            captureMouse(window, *state, true);
        return;
    }
    if (state->cash->active()) {
        if (!state->mouseCaptured && !ImGui::GetIO().WantCaptureMouse)
            captureMouse(window, *state, true);
        return;
    }
    if (state->camera->transitioning() || state->drink->opening() || ImGui::GetIO().WantCaptureMouse) return;
    double x = 0.0, y = 0.0;
    int width = 0, height = 0, windowWidth = 0, windowHeight = 0;
    glfwGetCursorPos(window, &x, &y);
    glfwGetFramebufferSize(window, &width, &height);
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) return;
    const float cursorX = static_cast<float>(x * width / windowWidth);
    const int sceneWidth = static_cast<int>(width * state->viewportRatio);
    if (cursorX < sceneWidth)
        state->interaction->click(cursorX, static_cast<float>(y * height / windowHeight),
                                  sceneWidth, height, *state->camera);
}

void cursorPositionCallback(GLFWwindow* window, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    AppState* state = appState(window);
    if (state && state->drink->inspecting()) {
        if (state->hasPreviousCursor && !ImGui::GetIO().WantCaptureMouse && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            state->drink->inspectRotate(static_cast<float>(x - state->previousX), static_cast<float>(y - state->previousY));
        state->previousX = x; state->previousY = y; state->hasPreviousCursor = true;
        return;
    }
    if (state && (state->camera->presentationActive())) return;
    if (!state || !state->mouseCaptured || (state->camera->interacting() && !state->cash->active())) return;
    if (state->cash->busy()) { state->hasPreviousCursor = false; return; }
    if (state->hasPreviousCursor)
        state->camera->look(static_cast<float>(x - state->previousX),
                            static_cast<float>(y - state->previousY));
    state->previousX = x;
    state->previousY = y;
    state->hasPreviousCursor = true;
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
    if (auto* state = appState(window); state && state->drink->inspecting() && !ImGui::GetIO().WantCaptureMouse) state->drink->inspectZoom(static_cast<float>(yOffset));
}

void focusCallback(GLFWwindow* window, int focused) {
    ImGui_ImplGlfw_WindowFocusCallback(window, focused);
    if (AppState* state = appState(window)) {
        state->hasPreviousCursor = false;
        if (!focused) captureMouse(window, *state, false);
    }
}

void performAction(GLFWwindow* window, int key) {
    AppState* state = appState(window);
    if (!state) return;
    if (key == GLFW_KEY_X) { state->options->xray = !state->options->xray; return; }
    if (key == GLFW_KEY_Z) { state->options->wireframe = !state->options->wireframe; return; }
    if (key == GLFW_KEY_V) { state->options->cinematic = !state->options->cinematic; return; }
    if (key == GLFW_KEY_C && !state->cash->active() && !state->animation->active() && !state->drink->opening()) {
        if (state->drink->inspecting()) state->drink->toggleInspect();
        state->camera->toggleExternal();
        captureMouse(window, *state, !state->camera->external());
        return;
    }
    if (key == GLFW_KEY_R && !state->cash->active() && !state->camera->presentationActive() && !state->camera->interacting()) {
        if (state->drink->toggleInspect()) captureMouse(window, *state, !state->drink->inspecting());
        return;
    }
    if (key == GLFW_KEY_ESCAPE && state->drink->inspecting()) {
        state->drink->toggleInspect();
        captureMouse(window, *state, true);
        return;
    }
    if (state->drink->inspecting() || state->camera->presentationActive()) return;
    if (key == GLFW_KEY_ESCAPE) {
        if (state->camera->interacting()) {
            if (state->animation->active() || state->animation->pressing() || state->drink->opening()) return;
            state->exitRequested = true;
            state->cash->cancel(*state->payment);
        } else {
            captureMouse(window, *state, false);
        }
        return;
    }

    if (key == GLFW_KEY_E && state->cash->state() == CashInteraction::State::AwaitingCollection) {
        state->cash->collect();
        return;
    }
    if (key == GLFW_KEY_F && state->drink->holding() && !state->cash->active() &&
        !state->animation->active() && !state->animation->pressing()) {
        state->drink->open();
        return;
    }
    if (key == GLFW_KEY_Q && state->drink->empty() && state->camera->canUseTrash()) {
        state->drink->throwAway({-4.9f, 1.0f, 0.4f}); return;
    }
    if (key == GLFW_KEY_E && !state->camera->interacting() && !state->cash->active() && !state->drink->opening()) {
        if (state->drink->placed() && state->camera->aimingAt(state->drink->placedPosition(), 0.35f, 4.5f)) {
            state->drink->takePlaced(); return;
        }
        if (state->drink->empty() && state->camera->canUseTrash()) {
            state->drink->throwAway({-4.9f, 1.0f, 0.4f}); return;
        }
        if (state->drink->holding() && state->camera->aimingAt({4.8f, 2.18f, 0.8f}, 1.1f, 4.5f)) {
            state->drink->putDown({4.8f, 2.49f, 0.8f}); return;
        }
        if (!state->drink->hasProduct() && state->animation->delivered() &&
            state->camera->aimingAt({-0.43f, 0.49f, 1.17f}, 0.55f, 4.5f)) {
            const int index = state->animation->productIndex();
            if (state->drink->pickUp(state->machine->products()[index], {-0.43f, 0.49f, 1.17f})) state->animation->collect();
            return;
        }
    }
    if (key == GLFW_KEY_E && state->drink->holding() && !state->camera->interacting()) return;
    if (key == GLFW_KEY_E && state->cash->state() == CashInteraction::State::Held &&
        state->camera->aimingAtPaymentSlot()) {
        state->cash->insert();
        return;
    }
    if (state->cash->active() || state->exitRequested || state->drink->opening()) return;
    if (state->camera->interacting() && !state->camera->transitioning() &&
        !state->animation->active() && !state->animation->pressing()) {
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_3) {
            const int values[] = {10000, 20000, 50000};
            state->cash->hold(values[key - GLFW_KEY_1]);
            return;
        }
        if (key == GLFW_KEY_E && state->animation->delivered() && !state->drink->hasProduct()) {
            const int index = state->animation->productIndex();
            if (state->drink->pickUp(state->machine->products()[index], {-0.43f, 0.49f, 1.17f})) {
                state->animation->collect();
                state->camera->endInteraction();
                captureMouse(window, *state, true);
            }
            return;
        }
    }
    if (key == GLFW_KEY_E && state->camera->beginInteraction()) {
        captureMouse(window, *state, false);
        return;
    }
    if (!state->camera->interacting() || state->camera->transitioning()) return;
    if (key == GLFW_KEY_LEFT) state->interaction->selectNext(-1);
    else if (key == GLFW_KEY_RIGHT) state->interaction->selectNext(1);
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) state->interaction->purchase();
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    if (action != GLFW_PRESS) return;
    if (ImGui::GetIO().WantCaptureKeyboard && key != GLFW_KEY_ESCAPE) return;
    performAction(window, key);
}

ImFont* loadVietnameseFont() {
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
    ImFont* font = io.Fonts->AddFontFromFileTTF(
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf", 17.0f, nullptr,
        glyphRanges);
    if (!font) {
        font = io.Fonts->AddFontFromFileTTF(
            "/System/Library/Fonts/Supplemental/Arial.ttf", 17.0f, nullptr,
            glyphRanges);
    }
    return font;
}

std::string money(float amount) {
    std::ostringstream stream;
    const std::string digits = std::to_string(static_cast<int>(std::lround(amount)));
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i && (digits.size() - i) % 3 == 0) stream << '.';
        stream << digits[i];
    }
    stream << "đ";
    return stream.str();
}

void drawContextHints(const AppState& state) {
    const auto& camera = *state.camera;
    const auto& cash = *state.cash;
    const auto& drink = *state.drink;
    const auto& animation = *state.animation;
    ImGui::SetNextWindowPos(ImVec2(16, 16));
    ImGui::SetNextWindowBgAlpha(0.94f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGui::Begin("Hướng dẫn theo tình huống", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(0.18f, 0.46f, 0.35f, 1), "MINT FRESH");
    ImGui::Separator();
    if (camera.presentationActive()) {
        ImGui::TextUnformatted(camera.external() ? "[C] Trở lại POV" : "Đang theo dõi sản phẩm...");
    } else if (drink.inspecting()) {
        ImGui::TextUnformatted("[Kéo chuột trái] Xoay sản phẩm");
        ImGui::TextUnformatted("[Cuộn chuột] Phóng to / thu nhỏ");
        ImGui::TextUnformatted("[R / Esc] Thoát quan sát");
    } else if (cash.busy()) {
        ImGui::TextUnformatted("Đang xử lý tiền, vui lòng chờ...");
    } else if (cash.state() == CashInteraction::State::AwaitingCollection) {
        ImGui::TextUnformatted("[E] Nhận tiền hoàn / tiền thừa");
    } else if (cash.state() == CashInteraction::State::Held) {
        ImGui::TextUnformatted("[Chuột] Nhìn vào khe nhận tiền");
        if (camera.aimingAtPaymentSlot()) ImGui::TextUnformatted("[E] Đưa tiền vào khe");
        ImGui::TextUnformatted("[Esc] Hủy giao dịch");
    } else if (animation.active() || animation.pressing() || drink.opening() || camera.transitioning()) {
        ImGui::TextUnformatted("Đang thực hiện thao tác...");
    } else if (camera.interacting()) {
        ImGui::TextUnformatted("[1] 10.000đ  [2] 20.000đ  [3] 50.000đ");
        ImGui::TextUnformatted("[Chuột trái] Nhấn nút trên máy");
        ImGui::TextUnformatted("[Mũi tên trái / phải] Chọn đồ uống");
        if (animation.delivered() && !drink.hasProduct()) ImGui::TextUnformatted("[E] Nhận nước trong khay");
        else if (!animation.delivered()) ImGui::TextUnformatted("[Enter] Xác nhận mua");
        ImGui::TextUnformatted("[Esc] Hủy / hoàn tiền / thoát");
        if (const auto* product = state.machine->selectedProduct())
            ImGui::Text("%s %s | %s | Còn %d", productCode(state.machine->selectedIndex()).c_str(),
                product->name.c_str(), money(product->price).c_str(), product->stock);
        ImGui::Text("Số dư: %s", money(state.payment->balance()).c_str());
        ImGui::TextWrapped("%s", state.machine->status());
    } else {
        ImGui::TextUnformatted("[W A S D] Di chuyển  [Shift] Đi nhanh");
        ImGui::TextUnformatted("[Chuột] Nhìn xung quanh");
        if (!state.mouseCaptured) ImGui::TextUnformatted("[Chuột trái] Tiếp tục điều khiển POV");
        if (drink.placed() && camera.aimingAt(drink.placedPosition(), 0.35f, 4.5f))
            ImGui::TextUnformatted("[E] Nhặt đồ uống trên bàn");
        else if (drink.holding() && camera.aimingAt({4.8f, 2.18f, 0.8f}, 1.1f, 4.5f))
            ImGui::TextUnformatted("[E] Đặt đồ uống lên bàn");
        else if (drink.empty() && camera.canUseTrash())
            ImGui::TextUnformatted("[E / Q] Bỏ vỏ vào sọt");
        else if (!drink.hasProduct() && animation.delivered() && camera.aimingAt({-0.43f, 0.49f, 1.17f}, 0.55f, 4.5f))
            ImGui::TextUnformatted("[E] Nhận nước trong khay");
        else if (!drink.holding() && camera.canInteract()) ImGui::TextUnformatted("[E] Tương tác với máy");
        if (drink.holding() && !drink.pickingUp()) {
            ImGui::TextUnformatted("[R] Quan sát sản phẩm");
            if (!drink.opened()) ImGui::TextUnformatted("[F] Khui lon / mở nắp");
            else if (!drink.empty()) ImGui::TextUnformatted("[Giữ chuột trái / E] Uống");
        }
    }
    if (drink.hasProduct()) {
        ImGui::Separator();
        ImGui::TextUnformatted(drink.product().name.c_str());
        char volume[40];
        std::snprintf(volume, sizeof(volume), "%.0f / %.0f ml", drink.remaining(), drink.capacity());
        ImGui::ProgressBar(drink.remaining() / drink.capacity(), ImVec2(265, 22), volume);
    }
    if (!cash.active() && !animation.active() && !drink.opening()) ImGui::TextUnformatted("[C] Camera ngoài / POV");
    ImGui::Separator();
    ImGui::TextUnformatted("[Z] Solid / Wireframe  [X] X-Ray");
    ImGui::TextUnformatted("[V] Bật / tắt camera điện ảnh");
    ImGui::End();
    ImGui::PopStyleVar(2);
}


}

int main() {
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window =
        glfwCreateWindow(1600, 960, "MINT FRESH | Beverage Studio", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create an OpenGL 3.3 window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImFont* vietnameseFont = loadVietnameseFont();
    if (!vietnameseFont) {
        std::cerr << "Không tìm thấy phông chữ hỗ trợ tiếng Việt trong macOS.\n";
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    ImGui::GetIO().FontDefault = vietnameseFont;
    ImGui::StyleColorsLight();
    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.13f, 0.29f, 0.24f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.48f, 0.41f, 1.0f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.97f, 0.96f, 0.91f, 0.97f);
    colors[ImGuiCol_Header] = ImVec4(0.74f, 0.86f, 0.77f, 1);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.64f, 0.81f, 0.70f, 1);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.55f, 0.75f, 0.64f, 1);
    colors[ImGuiCol_Button] = ImVec4(0.71f, 0.86f, 0.76f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.58f, 0.79f, 0.66f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.43f, 0.69f, 0.55f, 1.0f);
    ImGui::GetStyle().FrameRounding = 7.0f;
    ImGui::GetStyle().GrabRounding = 7.0f;
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init("#version 330");

    int exitCode = 0;
    try {
        VendingMachine machine;
        Payment payment;
        Animation animation;
        Camera camera;
        Interaction interaction(machine, payment, animation);
        CashInteraction cash;
        HeldDrink drink;
        SceneOptions options;
        Sound sound;
        AppState state;
        state.camera = &camera;
        state.interaction = &interaction;
        state.cash = &cash;
        state.payment = &payment;
        state.animation = &animation;
        state.drink = &drink;
        state.options = &options;
        state.machine = &machine;
        glfwSetWindowUserPointer(window, &state);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetWindowFocusCallback(window, focusCallback);
        captureMouse(window, state, true);

        Renderer renderer(VM3D_SHADER_DIR);
        double previousTime = glfwGetTime();
        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();
            const double currentTime = glfwGetTime();
            const float deltaTime = std::clamp(static_cast<float>(currentTime - previousTime), 0.0f, 0.25f);
            animation.update(deltaTime);
            if (animation.takeArrival()) {
                machine.setStatus("Giao dịch thành công! Đồ uống đã ở khay nhận hàng.");
                cash.returnChange(payment);
            }
            if (const int effect = animation.takeSound()) sound.effect(effect);
            const auto previousCashState = cash.state();
            const bool wasChange = cash.isChange();
            cash.update(deltaTime, payment);
            if (previousCashState == CashInteraction::State::Inserting && !cash.active())
                machine.setStatus("Máy đã nhận tiền. Có thể nạp thêm hoặc chọn đồ uống.");
            if (previousCashState == CashInteraction::State::Collecting && !cash.active())
                machine.setStatus(wasChange ? "Đã trả tiền thừa. Đồ uống đang ở khay nhận hàng." : "Đã nhận lại tiền. Giao dịch đã hủy.");
            if (cash.takeSound()) sound.cashMotor();
            if (state.exitRequested && !cash.active()) {
                if (!cash.cancel(payment)) {
                    state.exitRequested = false;
                    camera.endInteraction();
                    captureMouse(window, state, true);
                }
            }
            const bool canUseDrink = !cash.active() && !animation.active() && !animation.pressing() &&
                                     !camera.interacting() && !camera.presentationActive() && !drink.inspecting() &&
                                     glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
                                     (state.mouseCaptured && !ImGui::GetIO().WantCaptureMouse);
            drink.update(deltaTime, canUseDrink &&
                (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS));
            if (const int effect = drink.takeSound()) sound.effect(effect);
            camera.setDrinkMotion(drink.drinkBlend(), drink.time());
            camera.setPickupMotion(drink.pickingUp() && !drink.pickupFromTable() ?
                std::sin(drink.pickupProgress() * 3.14159265f) : 0.0f);
            if (!camera.presentationActive()) camera.focusPayment(cash.active());
            if (camera.interacting() && !camera.transitioning() && !cash.active() && state.mouseCaptured)
                captureMouse(window, state, false);
            const bool walking = state.mouseCaptured && !camera.interacting() && !camera.presentationActive() && !drink.inspecting() &&
                                 glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
                                 !ImGui::GetIO().WantCaptureKeyboard;
            const auto down = [&](int key) { return walking && glfwGetKey(window, key) == GLFW_PRESS; };
            int frameWidth = 0, frameHeight = 0;
            glfwGetFramebufferSize(window, &frameWidth, &frameHeight);
            if (frameHeight > 0)
                camera.setInteractionAspect(static_cast<float>(frameWidth) * sceneRatio / frameHeight);
            camera.update(deltaTime, static_cast<float>(down(GLFW_KEY_W)) - down(GLFW_KEY_S),
                          static_cast<float>(down(GLFW_KEY_D)) - down(GLFW_KEY_A),
                          down(GLFW_KEY_LEFT_SHIFT) || down(GLFW_KEY_RIGHT_SHIFT));
            const auto moving = animation.productIndex() >= 0 ?
                animation.deliveryPosition(machine.productPosition(animation.productIndex())) : glm::vec3(0, 2.65f, 0);
            camera.updatePresentation(deltaTime, options.cinematic, animation.active(), animation.progress(), moving);
            const float targetRatio = camera.interacting() && !camera.presentationActive() ?
                1.0f - (1.0f - sceneRatio) * camera.interactionBlend() : 1.0f;
            state.viewportRatio += (targetRatio - state.viewportRatio) *
                                   (1.0f - std::exp(-12.0f * deltaTime));
            previousTime = currentTime;

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            int width = 0;
            int height = 0;
            int windowWidth = 0;
            int windowHeight = 0;
            glfwGetFramebufferSize(window, &width, &height);
            glfwGetWindowSize(window, &windowWidth, &windowHeight);
            const int sceneWidth = static_cast<int>(static_cast<float>(width) * state.viewportRatio);
            const float sceneWidthPoints =
                static_cast<float>(windowWidth) * state.viewportRatio;
            double cursorX = windowWidth * state.viewportRatio * 0.5, cursorY = windowHeight * 0.5;
            if (!state.mouseCaptured) glfwGetCursorPos(window, &cursorX, &cursorY);
            if (!cash.active() && !camera.transitioning() && width > 0 && height > 0 &&
                windowWidth > 0 && windowHeight > 0 && cursorX < sceneWidthPoints)
                interaction.hover(static_cast<float>(cursorX * width / windowWidth),
                                  static_cast<float>(cursorY * height / windowHeight), sceneWidth, height, camera);
            else machine.hover(-1);
            renderer.render(machine, animation, camera, cash, payment, drink, options, sceneWidth, height);
            if (drink.pickingUp() && !camera.interacting() && !state.mouseCaptured) captureMouse(window, state, true);
            if (cash.active() && !state.mouseCaptured && glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
                camera.focusPayment(true);
                captureMouse(window, state, true);
            }
            drawContextHints(state);
            if ((!camera.interacting() || cash.state() == CashInteraction::State::Held) &&
                state.mouseCaptured && !camera.presentationActive() && !drink.inspecting()) {
                auto* draw = ImGui::GetForegroundDrawList();
                draw->AddCircleFilled(ImVec2(sceneWidthPoints * 0.5f, windowHeight * 0.5f), 2,
                                      IM_COL32(35, 90, 65, 220));
            }
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    } catch (const std::exception& error) {
        std::cerr << "Application error: " << error.what() << '\n';
        exitCode = 1;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return exitCode;
}
