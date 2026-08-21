#include "platform/Platform.hpp"

#include <GLFW/glfw3.h>

namespace
{
    GLFWwindow* gWindow = nullptr;
}

namespace OpenLRR::Platform
{
    bool Initialise()
    {
        return glfwInit() == GLFW_TRUE;
    }

    void Shutdown()
    {
        if (gWindow != nullptr) {
            glfwDestroyWindow(gWindow);
            gWindow = nullptr;
        }

        glfwTerminate();
    }

    bool CreateWindow(int width, int height, const char* title)
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        gWindow = glfwCreateWindow(
            width,
            height,
            title,
            nullptr,
            nullptr
        );

        return gWindow != nullptr;
    }

    void PollEvents()
    {
        glfwPollEvents();
    }

    bool ShouldClose()
    {
        return gWindow == nullptr ||
               glfwWindowShouldClose(gWindow);
    }
}
