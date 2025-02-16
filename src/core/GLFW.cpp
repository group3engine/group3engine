#include "GLFW.hpp"

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

void PollInputEvents() {
    // TODO: More polling handling

    // Register previous key states
    for (uint16_t i = 0; i < static_cast<uint16_t>(KEY::_LAST); ++i) {
        gInputData.keyboard.previousKeyState[i] =
            gInputData.keyboard.currentKeyState[i];
    }

    glfwPollEvents();
}

static void KeyCallback([[maybe_unused]] GLFWwindow *window,
                        int key,
                        [[maybe_unused]] int scancode,
                        int action,
                        [[maybe_unused]] int mods) {
    if (key < 0) {
        return;
    }

    if (action == GLFW_RELEASE) {
        gInputData.keyboard.currentKeyState[key] = 0;
    } else if (action == GLFW_PRESS) {
        gInputData.keyboard.currentKeyState[key] = 1;
    }
}

void Platform::StartUp(int windowWidth, int windowHeight) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Platform::get().window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);

    if (!Platform::get().window) {
        SPDLOG_ERROR("Failed to create GLFW window");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(Platform::get().window, &KeyCallback);
}

void Platform::ShutDown() {
    glfwDestroyWindow(Platform::get().window);
    glfwTerminate();
}
