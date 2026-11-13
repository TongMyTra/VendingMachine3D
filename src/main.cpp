#include "Animation.h"
#include "Camera.h"
#include "CashInteraction.h"
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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr float sceneRatio = 0.66f;

struct AppState {
    Camera* camera{};
    Interaction* interaction{};
    CashInteraction* cash{};
    Payment* payment{};
    Animation* animation{};
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
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    AppState* state = appState(window);
    if (!state || button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return;
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
    if (state->camera->transitioning() || ImGui::GetIO().WantCaptureMouse) return;
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
    if (!state || !state->mouseCaptured || (state->camera->interacting() && !state->cash->active())) return;
    if (state->hasPreviousCursor)
        state->camera->look(static_cast<float>(x - state->previousX),
                            static_cast<float>(y - state->previousY));
    state->previousX = x;
    state->previousY = y;
    state->hasPreviousCursor = true;
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
}

void focusCallback(GLFWwindow* window, int focused) {
    ImGui_ImplGlfw_WindowFocusCallback(window, focused);
    if (AppState* state = appState(window)) {
        state->hasPreviousCursor = false;
        if (!focused) captureMouse(window, *state, false);
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    AppState* state = appState(window);
    if (!state || action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        if (state->camera->interacting()) {
            if (state->animation->active()) return;
            state->exitRequested = true;
            state->cash->cancel(*state->payment);
        } else {
            captureMouse(window, *state, false);
        }
        return;
    }
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    if (key == GLFW_KEY_E && state->cash->state() == CashInteraction::State::Held &&
        state->mouseCaptured && state->camera->aimingAtPaymentSlot()) {
        state->cash->insert();
        return;
    }
    if (state->cash->active() || state->exitRequested) return;
    if (key == GLFW_KEY_E && state->mouseCaptured && state->camera->beginInteraction()) {
        captureMouse(window, *state, false);
        return;
    }
    if (!state->camera->interacting() || state->camera->transitioning()) return;
    if (key == GLFW_KEY_LEFT) state->interaction->selectNext(-1);
    else if (key == GLFW_KEY_RIGHT) state->interaction->selectNext(1);
    else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) state->interaction->purchase();
}

void drawWalkHud(const Camera& camera, const CashInteraction& cash, bool captured, float width, float height) {
    const char* hint = cash.busy() ? "Đang xử lý tiền..." :
        cash.state() == CashInteraction::State::Held ?
        (camera.aimingAtPaymentSlot() ? "E: Đưa tiền vào khe   |   ESC: Hủy và hoàn tiền" :
         "Di chuột nhìn vào khe nhận tiền   |   ESC: Hủy") : camera.interacting() ? "ESC: Trở lại di chuyển" :
        !captured ? "Click để tiếp tục di chuyển" :
        camera.canInteract() ? "E: Tương tác với MINT FRESH" :
        "WASD: Di chuyển   |   Chuột: Nhìn   |   Shift: Đi nhanh";
    ImGui::SetNextWindowPos(ImVec2(20.0f, height - 65.0f));
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::Begin("Hướng dẫn", nullptr, ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextUnformatted(hint);
    ImGui::End();
    if ((!camera.interacting() || cash.state() == CashInteraction::State::Held) && captured) {
        const ImVec2 center(width * 0.5f, height * 0.5f);
        auto* draw = ImGui::GetForegroundDrawList();
        draw->AddCircleFilled(center, 3.5f, IM_COL32(25, 70, 53, 200));
        draw->AddCircleFilled(center, 1.5f, IM_COL32(245, 250, 240, 255));
    }
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

void drawInterface(VendingMachine& machine, Payment& payment, Interaction& interaction,
                   CashInteraction& cash, Camera& camera, bool vending,
                   float panelX, float panelWidth, float panelHeight) {
    ImGui::SetNextWindowPos(ImVec2(panelX, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 18.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.97f, 0.96f, 0.91f, 1.0f));
    ImGui::Begin("Máy bán hàng | Cửa hàng", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored(ImVec4(0.18f, 0.46f, 0.35f, 1.0f), "MINT FRESH");
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 130.0f);
    ImGui::TextDisabled("MÁY SỐ 03");
    ImGui::Spacing();
    ImGui::TextUnformatted("Chọn sản phẩm bạn yêu thích");
    ImGui::TextDisabled("Chọn món, nạp tiền và nhận hàng bên dưới.");
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.93f, 0.94f, 0.88f, 1.0f));
    ImGui::BeginChild("Danh sách sản phẩm", ImVec2(0.0f, panelHeight * 0.43f), true);
    if (ImGui::BeginTable("Lưới sản phẩm", 2,
                          ImGuiTableFlags_SizingStretchSame |
                              ImGuiTableFlags_BordersInnerV)) {
        for (int i = 0; i < static_cast<int>(machine.products().size()); ++i) {
            const Product& product = machine.products()[static_cast<std::size_t>(i)];
            if ((i % 2) == 0) {
                ImGui::TableNextRow();
            }
            ImGui::TableSetColumnIndex(i % 2);
            ImGui::PushID(product.id);
            if (i == machine.selectedIndex()) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.81f, 0.69f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                      ImVec4(0.52f, 0.76f, 0.63f, 1.0f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.86f, 0.90f, 0.82f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                                      ImVec4(0.75f, 0.85f, 0.73f, 1.0f));
            }
            const std::string label =
                product.name + "\n" + money(product.price) + "  /  Còn " +
                std::to_string(product.stock);
            if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 60.0f))) {
                machine.select(i);
                machine.setStatus("Đã chọn sản phẩm. Hãy nạp tiền và bấm Mua hàng.");
            }
            ImGui::PopStyleColor(2);
            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    const Product* selected = machine.selectedProduct();
    if (selected) {
        ImGui::TextDisabled("SẢN PHẨM ĐÃ CHỌN");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 115.0f);
        ImGui::TextColored(ImVec4(0.18f, 0.46f, 0.35f, 1.0f), "CÒN %d",
                           selected->stock);
        ImGui::Text("%s", selected->name.c_str());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 102.0f);
        ImGui::Text("%s", money(selected->price).c_str());
    }
    ImGui::Text("SỐ DƯ   %s", money(payment.balance()).c_str());
    ImGui::SameLine();
    if (payment.change() > 0.001f) {
        ImGui::TextColored(ImVec4(0.18f, 0.46f, 0.35f, 1.0f),
                           "TIỀN THỪA %s", money(payment.change()).c_str());
    }
    ImGui::TextWrapped("%s", machine.status());

    ImGui::BeginDisabled(cash.active() || camera.transitioning() || vending);
    const float buttonWidth = (ImGui::GetContentRegionAvail().x - 16.0f) / 3.0f;
    if (ImGui::Button("10.000đ", ImVec2(buttonWidth, 36.0f))) cash.hold(10000);
    ImGui::SameLine();
    if (ImGui::Button("20.000đ", ImVec2(buttonWidth, 36.0f))) cash.hold(20000);
    ImGui::SameLine();
    if (ImGui::Button("50.000đ", ImVec2(buttonWidth, 36.0f))) cash.hold(50000);
    ImGui::EndDisabled();
    if (cash.active()) ImGui::TextWrapped("Cầm tiền: hướng chuột vào khe, nhấn E để nạp.");
    ImGui::BeginDisabled(cash.active() || camera.transitioning() || vending);

    ImGui::Spacing();
    if (ImGui::Button("MUA HÀNG", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, 44.0f))) {
        interaction.purchase();
    }
    ImGui::SameLine();
    if (ImGui::Button("TRẢ LẠI TIỀN", ImVec2(-1.0f, 44.0f))) {
        cash.cancel(payment);
        machine.setStatus("Đang hoàn tiền qua khe trả tiền...");
    }
    ImGui::Spacing();
    ImGui::EndDisabled();
    ImGui::TextDisabled("ESC: DI CHUYỂN  /  MŨI TÊN: CHỌN MÓN  /  ENTER: MUA");
    ImGui::End();
    ImGui::PopStyleColor();
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
        Sound sound;
        AppState state;
        state.camera = &camera;
        state.interaction = &interaction;
        state.cash = &cash;
        state.payment = &payment;
        state.animation = &animation;
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
            const float deltaTime = std::clamp(static_cast<float>(currentTime - previousTime), 0.0f, 0.05f);
            animation.update(deltaTime);
            const auto previousCashState = cash.state();
            cash.update(deltaTime, payment);
            if (previousCashState == CashInteraction::State::Inserting && !cash.active())
                machine.setStatus("Máy đã nhận tiền. Có thể nạp thêm hoặc chọn đồ uống.");
            if (previousCashState == CashInteraction::State::Returning && !cash.active())
                machine.setStatus("Đã nhận lại tiền. Giao dịch đã hủy.");
            if (cash.takeSound()) sound.cashMotor();
            if (state.exitRequested && !cash.busy()) {
                if (!cash.cancel(payment)) {
                    state.exitRequested = false;
                    camera.endInteraction();
                    captureMouse(window, state, true);
                }
            }
            camera.focusPayment(cash.active());
            if (camera.interacting() && !camera.transitioning() && !cash.active() && state.mouseCaptured)
                captureMouse(window, state, false);
            const bool walking = state.mouseCaptured && !camera.interacting() &&
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
            const float targetRatio = camera.interacting() ?
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
            const float panelWidthPoints =
                static_cast<float>(windowWidth) - sceneWidthPoints;
            renderer.render(machine, animation, camera, cash, payment, sceneWidth, height);
            if (camera.interacting() && !camera.transitioning())
                drawInterface(machine, payment, interaction, cash, camera, animation.active(), sceneWidthPoints,
                              panelWidthPoints, static_cast<float>(windowHeight));
            if (cash.active() && !state.mouseCaptured && glfwGetWindowAttrib(window, GLFW_FOCUSED)) {
                camera.focusPayment(true);
                captureMouse(window, state, true);
            }
            drawWalkHud(camera, cash, state.mouseCaptured, sceneWidthPoints,
                        static_cast<float>(windowHeight));
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
