#include "renderer/VulkanRenderer.hpp"
#include "platform/Platform.hpp"

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

namespace
{
    VkInstance gInstance = VK_NULL_HANDLE;
    VkSurfaceKHR gSurface = VK_NULL_HANDLE;
}

namespace OpenLRR::Renderer
{
    bool InitialiseVulkan()
    {
        unsigned extensionCount = 0;

        const char** extensions =
            Platform::GetRequiredVulkanExtensions(
                &extensionCount
            );

        if (extensions == nullptr ||
            extensionCount == 0)
        {
            std::cerr
                << "GLFW did not provide Vulkan extensions\n";
            return false;
        }

        VkApplicationInfo applicationInfo{};
        applicationInfo.sType =
            VK_STRUCTURE_TYPE_APPLICATION_INFO;

        applicationInfo.pApplicationName = "OpenLRR";
        applicationInfo.applicationVersion =
            VK_MAKE_VERSION(0, 1, 0);

        applicationInfo.pEngineName = "OpenLRR";
        applicationInfo.engineVersion =
            VK_MAKE_VERSION(0, 1, 0);

        applicationInfo.apiVersion =
            VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        createInfo.pApplicationInfo =
            &applicationInfo;

        createInfo.enabledExtensionCount =
            extensionCount;

        createInfo.ppEnabledExtensionNames =
            extensions;

        if (vkCreateInstance(
                &createInfo,
                nullptr,
                &gInstance) != VK_SUCCESS)
        {
            std::cerr
                << "Failed to create Vulkan instance\n";
            return false;
        }

        if (!Platform::CreateVulkanSurface(
                gInstance,
                &gSurface))
        {
            std::cerr
                << "Failed to create Vulkan surface\n";

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;

            return false;
        }

        std::cout
            << "Vulkan instance created successfully\n";

        std::cout
            << "Vulkan window surface created successfully\n";

        return true;
    }

    void ShutdownVulkan()
    {
        if (gSurface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;
        }

        if (gInstance != VK_NULL_HANDLE) {
            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;
        }
    }
}
