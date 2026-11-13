cmake_minimum_required(VERSION 3.24)
project(VendingMachine3D VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
find_package(OpenGL REQUIRED)
find_package(Python COMPONENTS Interpreter REQUIRED)

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import glad, jinja2"
    RESULT_VARIABLE GLAD_PYTHON_STATUS
    OUTPUT_QUIET
    ERROR_QUIET
)
if(NOT GLAD_PYTHON_STATUS EQUAL 0)
    message(FATAL_ERROR
        "GLAD's Python generator is missing. Create a virtualenv, install glad2, "
        "and configure with -DPython_EXECUTABLE=/path/to/venv/bin/python."
    )
endif()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glad)
include(${glad_SOURCE_DIR}/cmake/GladConfig.cmake)
set(GLAD_SOURCES_DIR "${glad_SOURCE_DIR}")
glad_add_library(glad_gl_core_33 REPRODUCIBLE API gl:core=3.3)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

add_executable(VendingMachine3D
    src/main.cpp
    src/VendingMachine.cpp
    src/Product.cpp
    src/Camera.cpp
    src/Renderer.cpp
    src/Payment.cpp
    src/CashInteraction.cpp
    src/Sound.cpp
    src/HeldDrink.cpp
    src/Interaction.cpp
    src/Animation.cpp
)
target_include_directories(VendingMachine3D PRIVATE include)
target_link_libraries(VendingMachine3D PRIVATE
    glad_gl_core_33
    glm::glm
    imgui
    OpenGL::GL
)
target_compile_definitions(VendingMachine3D PRIVATE
    VM3D_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)

if(APPLE)
    target_link_libraries(VendingMachine3D PRIVATE "-framework AudioToolbox")
    target_compile_definitions(VendingMachine3D PRIVATE GL_SILENCE_DEPRECATION)
endif()

if(MSVC)
    target_compile_options(VendingMachine3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(VendingMachine3D PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(CameraTests tests/CameraTests.cpp src/Camera.cpp)
    target_include_directories(CameraTests PRIVATE include)
    target_link_libraries(CameraTests PRIVATE glm::glm)
    # Keep assertions active in Release builds as well.
    if(MSVC)
        target_compile_options(CameraTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(CameraTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME CameraControls COMMAND CameraTests)
endif()

if(BUILD_TESTING)
    add_executable(CashTests tests/CashTests.cpp src/CashInteraction.cpp src/Payment.cpp)
    target_include_directories(CashTests PRIVATE include)
    if(MSVC)
        target_compile_options(CashTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(CashTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME CashWorkflow COMMAND CashTests)
endif()

if(BUILD_TESTING)
    add_executable(VendingTests tests/VendingTests.cpp src/Animation.cpp src/Payment.cpp
        src/CashInteraction.cpp src/VendingMachine.cpp src/Product.cpp src/Interaction.cpp src/Camera.cpp)
    target_include_directories(VendingTests PRIVATE include)
    target_link_libraries(VendingTests PRIVATE glm::glm)
    if(MSVC)
        target_compile_options(VendingTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(VendingTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME VendingWorkflow COMMAND VendingTests)
endif()

if(BUILD_TESTING)
    add_executable(HeldDrinkTests tests/HeldDrinkTests.cpp src/HeldDrink.cpp src/Product.cpp)
    target_include_directories(HeldDrinkTests PRIVATE include)
    target_link_libraries(HeldDrinkTests PRIVATE glm::glm)
    if(MSVC)
        target_compile_options(HeldDrinkTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(HeldDrinkTests PRIVATE -UNDEBUG)
    endif()
    add_test(NAME HeldDrinkWorkflow COMMAND HeldDrinkTests)
endif()

if(BUILD_TESTING AND APPLE)
    add_executable(RendererSmokeTests tests/RendererSmokeTests.cpp src/Renderer.cpp
        src/Animation.cpp src/Camera.cpp src/CashInteraction.cpp src/Payment.cpp
        src/HeldDrink.cpp src/Product.cpp src/VendingMachine.cpp)
    target_include_directories(RendererSmokeTests PRIVATE include)
    target_link_libraries(RendererSmokeTests PRIVATE glad_gl_core_33 glm::glm OpenGL::GL)
    target_compile_definitions(RendererSmokeTests PRIVATE GL_SILENCE_DEPRECATION
        VM3D_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    add_test(NAME RendererModes COMMAND RendererSmokeTests)
    set_tests_properties(RendererModes PROPERTIES SKIP_RETURN_CODE 77)
endif()
