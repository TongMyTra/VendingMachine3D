#include "Animation.h"
#include "Camera.h"
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

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
constexpr float sceneRatio = 0.66f;

struct AppState {
    Camera* camera{};
    Interaction* interaction{};
    bool orbiting{};
    bool hasPreviousCursor{};
    double previousX{};
    double previousY{};
};

AppState* appState(GLFWwindow* window) {
    return static_cast<AppState*>(glfwGetWindowUserPointer(window));
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    AppState* state = appState(window);
    if (!state) {
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        double x = 0.0;
        int windowWidth = 0;
        glfwGetCursorPos(window, &x, nullptr);
        glfwGetWindowSize(window, &windowWidth, nullptr);
        state->orbiting = action == GLFW_PRESS &&
                          x < static_cast<double>(windowWidth) * sceneRatio &&
                          !ImGui::GetIO().WantCaptureMouse;
        state->hasPreviousCursor = false;
        return;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS &&
        !ImGui::GetIO().WantCaptureMouse) {
        double x = 0.0;
        double y = 0.0;
        int width = 0;
        int height = 0;
        glfwGetCursorPos(window, &x, &y);
        glfwGetFramebufferSize(window, &width, &height);
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        if (windowWidth > 0 && windowHeight > 0) {
            const float scaleX = static_cast<float>(width) / static_cast<float>(windowWidth);
            const float scaleY = static_cast<float>(height) / static_cast<float>(windowHeight);
            const float cursorX = static_cast<float>(x) * scaleX;
            const int sceneWidth = static_cast<int>(static_cast<float>(width) * sceneRatio);
            if (cursorX < static_cast<float>(sceneWidth)) {
                state->interaction->click(cursorX, static_cast<float>(y) * scaleY,
                                          sceneWidth, height, *state->camera);
            }
        }
    }
}

void cursorPositionCallback(GLFWwindow* window, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    AppState* state = appState(window);
    if (!state || !state->orbiting) {
        return;
    }
    if (state->hasPreviousCursor) {
        state->camera->orbit(static_cast<float>(x - state->previousX),
                             static_cast<float>(state->previousY - y));
    }
    state->previousX = x;
    state->previousY = y;
    state->hasPreviousCursor = true;
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);
    AppState* state = appState(window);
    double x = 0.0;
    int windowWidth = 0;
    glfwGetCursorPos(window, &x, nullptr);
    glfwGetWindowSize(window, &windowWidth, nullptr);
    if (state && !ImGui::GetIO().WantCaptureMouse &&
        x < static_cast<double>(windowWidth) * sceneRatio) {
        state->camera->zoom(static_cast<float>(yOffset));
    }
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    AppState* state = appState(window);
    if (!state || action != GLFW_PRESS || ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
        state->interaction->selectNext(-1);
    } else if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
        state->interaction->selectNext(1);
    } else if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) {
        state->interaction->purchase();
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
    stream << "$" << std::fixed << std::setprecision(2) << amount;
    return stream.str();
}

void drawInterface(VendingMachine& machine, Payment& payment, Interaction& interaction,
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

    const float buttonWidth = (ImGui::GetContentRegionAvail().x - 16.0f) / 3.0f;
    if (ImGui::Button("Nạp $1", ImVec2(buttonWidth, 36.0f))) {
        payment.insert(1.0f);
        machine.setStatus("Đã nạp tiền. Tiền thừa sẽ được trả sau khi mua.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Nạp $2", ImVec2(buttonWidth, 36.0f))) {
        payment.insert(2.0f);
        machine.setStatus("Đã nạp tiền. Tiền thừa sẽ được trả sau khi mua.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Nạp $5", ImVec2(buttonWidth, 36.0f))) {
        payment.insert(5.0f);
        machine.setStatus("Đã nạp tiền. Tiền thừa sẽ được trả sau khi mua.");
    }

    ImGui::Spacing();
    if (ImGui::Button("MUA HÀNG", ImVec2(ImGui::GetContentRegionAvail().x * 0.62f, 44.0f))) {
        interaction.purchase();
    }
    ImGui::SameLine();
    if (ImGui::Button("TRẢ LẠI TIỀN", ImVec2(-1.0f, 44.0f))) {
        payment.refund();
        machine.setStatus("Đã trả lại số dư qua khe trả tiền.");
    }
    ImGui::Spacing();
    ImGui::TextDisabled("GÓC NHÌN 3D  /  GIỮ CHUỘT PHẢI ĐỂ XOAY  /  CUỘN ĐỂ THU PHÓNG");
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
        AppState state{&camera, &interaction};
        glfwSetWindowUserPointer(window, &state);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glfwSetScrollCallback(window, scrollCallback);
        glfwSetKeyCallback(window, keyCallback);

        Renderer renderer(VM3D_SHADER_DIR);
        double previousTime = glfwGetTime();
        while (glfwWindowShouldClose(window) == GLFW_FALSE) {
            glfwPollEvents();
            const double currentTime = glfwGetTime();
            animation.update(static_cast<float>(currentTime - previousTime));
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
            const int sceneWidth = static_cast<int>(static_cast<float>(width) * sceneRatio);
            const float sceneWidthPoints =
                static_cast<float>(windowWidth) * sceneRatio;
            const float panelWidthPoints =
                static_cast<float>(windowWidth) - sceneWidthPoints;
            renderer.render(machine, animation, camera, sceneWidth, height);
            drawInterface(machine, payment, interaction, sceneWidthPoints,
                          panelWidthPoints, static_cast<float>(windowHeight));
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
