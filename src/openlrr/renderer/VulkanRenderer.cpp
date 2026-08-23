#include "renderer/VulkanRenderer.hpp"
#include "renderer/PresentationViewport.hpp"
#include "platform/Platform.hpp"
#include "settings/Settings.hpp"


#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>

// OpenLRR screen-space convention:
//
//   origin = top-left
//   +X = right
//   +Y = down
//
// Higher-level 2D code should use this convention consistently.
// Vulkan-specific 3D projection differences are handled inside the renderer.

namespace
{
VkInstance gInstance = VK_NULL_HANDLE;
VkSurfaceKHR gSurface = VK_NULL_HANDLE;

VkPhysicalDevice gPhysicalDevice = VK_NULL_HANDLE;

VkDevice gDevice = VK_NULL_HANDLE;
VkQueue gGraphicsQueue = VK_NULL_HANDLE;
VkQueue gPresentQueue = VK_NULL_HANDLE;

bool gSwapchainResizePending = false;

std::chrono::steady_clock::time_point gLastFramebufferResizeEvent =
    std::chrono::steady_clock::now();

constexpr auto kResizeDebounceInterval =
    std::chrono::milliseconds(150);

VkSwapchainKHR gSwapchain = VK_NULL_HANDLE;

VkCommandPool gCommandPool = VK_NULL_HANDLE;
VkCommandBuffer gCommandBuffer = VK_NULL_HANDLE;

VkSemaphore gImageAvailableSemaphore = VK_NULL_HANDLE;
VkSemaphore gRenderFinishedSemaphore = VK_NULL_HANDLE;
VkFence gRenderFence = VK_NULL_HANDLE;

std::vector<bool> gSwapchainImageInitialised;

VkFormat gSwapchainImageFormat = VK_FORMAT_UNDEFINED;
VkExtent2D gSwapchainExtent{};

std::vector<VkImage> gSwapchainImages;
std::vector<VkImageView> gSwapchainImageViews;

VkImage gRenderImage = VK_NULL_HANDLE;
VkDeviceMemory gRenderImageMemory = VK_NULL_HANDLE;
VkImageView gRenderImageView = VK_NULL_HANDLE;

VkFormat gRenderImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
VkExtent2D gRenderImageExtent{};

bool gRenderImageInitialised = false;

uint32_t gCurrentImageIndex = 0;

OpenLRR::Renderer::PresentationViewport
    gCurrentPresentation{};

bool gFrameInProgress = false;

float gClearColor[4] = {
    0.08f,
    0.15f,
    0.32f,
    1.0f
};

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool IsComplete() const
        {
            return graphicsFamily.has_value() &&
                   presentFamily.has_value();
        }
    };

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities{};

    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr
    );

    std::vector<VkQueueFamilyProperties> queueFamilies(
        queueFamilyCount
    );

    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        queueFamilies.data()
    );

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags &
            VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;

        vkGetPhysicalDeviceSurfaceSupportKHR(
            device,
            i,
            gSurface,
            &presentSupport
        );

        if (presentSupport == VK_TRUE) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

SwapchainSupportDetails QuerySwapchainSupport(
    VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device,
        gSurface,
        &details.capabilities
    );

    uint32_t formatCount = 0;

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device,
        gSurface,
        &formatCount,
        nullptr
    );

    if (formatCount != 0) {
        details.formats.resize(formatCount);

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device,
            gSurface,
            &formatCount,
            details.formats.data()
        );
    }

    uint32_t presentModeCount = 0;

    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        gSurface,
        &presentModeCount,
        nullptr
    );

    if (presentModeCount != 0) {
        details.presentModes.resize(
            presentModeCount
        );

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            gSurface,
            &presentModeCount,
            details.presentModes.data()
        );
    }

    return details;
}

bool SupportsSwapchain(VkPhysicalDevice device)
{
    uint32_t extensionCount = 0;

    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extensionCount,
        nullptr
    );

    std::vector<VkExtensionProperties> extensions(
        extensionCount
    );

    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extensionCount,
        extensions.data()
    );

    for (const auto& extension : extensions) {
        if (std::strcmp(
                extension.extensionName,
                VK_KHR_SWAPCHAIN_EXTENSION_NAME
            ) == 0)
        {
            return true;
        }
    }

    return false;
}

int RatePhysicalDevice(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    if (properties.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_CPU)
    {
        return -1;
    }

    const QueueFamilyIndices queues =
        FindQueueFamilies(device);

    if (!queues.IsComplete()) {
        return -1;
    }

    if (!SupportsSwapchain(device)) {
        return -1;
    }

const SwapchainSupportDetails swapchainSupport =
    QuerySwapchainSupport(device);

if (swapchainSupport.formats.empty() ||
    swapchainSupport.presentModes.empty())
{
    return -1;
}

    int score = 0;

    switch (properties.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        score += 1000;
        break;

    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        score += 500;
        break;

    default:
        score += 100;
        break;
    }

    score += static_cast<int>(
        properties.limits.maxImageDimension2D / 1024
    );

    return score;
}

bool SelectPhysicalDevice()
{
    uint32_t deviceCount = 0;

    vkEnumeratePhysicalDevices(
        gInstance,
        &deviceCount,
        nullptr
    );

    if (deviceCount == 0) {
        std::cerr
            << "No Vulkan physical devices found\n";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);

    vkEnumeratePhysicalDevices(
        gInstance,
        &deviceCount,
        devices.data()
    );

    int bestScore = -1;

    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(
            device,
            &properties
        );

        const int score =
            RatePhysicalDevice(device);

        std::cout
            << "Vulkan device: "
            << properties.deviceName
            << " (score " << score << ")\n";

        if (score > bestScore) {
            bestScore = score;
            gPhysicalDevice = device;
        }
    }

    if (gPhysicalDevice == VK_NULL_HANDLE) {
        std::cerr
            << "No suitable Vulkan GPU found\n";
        return false;
    }

    VkPhysicalDeviceProperties selectedProperties{};
    vkGetPhysicalDeviceProperties(
        gPhysicalDevice,
        &selectedProperties
    );

    std::cout
        << "Selected Vulkan GPU: "
        << selectedProperties.deviceName
        << '\n';

    return true;
}

VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format ==
                VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats[0];
}

VkPresentModeKHR ChooseSwapchainPresentMode(
    const std::vector<VkPresentModeKHR>& presentModes)
{
    for (VkPresentModeKHR mode : presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D ChooseSwapchainExtent(
    const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    OpenLRR::Platform::GetFramebufferSize(
        &framebufferWidth,
        &framebufferHeight
    );

    VkExtent2D extent{};

    extent.width = std::clamp(
        static_cast<uint32_t>(framebufferWidth),
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );

    extent.height = std::clamp(
        static_cast<uint32_t>(framebufferHeight),
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}

uint32_t FindMemoryType(
    uint32_t typeFilter,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties{};

    vkGetPhysicalDeviceMemoryProperties(
        gPhysicalDevice,
        &memoryProperties
    );

    for (uint32_t i = 0;
         i < memoryProperties.memoryTypeCount;
         ++i)
    {
        const bool typeMatches =
            (typeFilter & (1u << i)) != 0;

        const bool propertiesMatch =
            (memoryProperties.memoryTypes[i].propertyFlags &
             properties) == properties;

        if (typeMatches && propertiesMatch) {
            return i;
        }
    }

    throw std::runtime_error(
        "Failed to find suitable Vulkan memory type"
    );
}

bool CreateRenderImage(
    int width,
    int height)
{
    if (width <= 0 || height <= 0) {
        std::cerr
            << "Invalid render image size\n";
        return false;
    }

    VkImageCreateInfo imageInfo{};

    imageInfo.sType =
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

    imageInfo.imageType =
        VK_IMAGE_TYPE_2D;

    imageInfo.extent.width =
        static_cast<uint32_t>(width);

    imageInfo.extent.height =
        static_cast<uint32_t>(height);

    imageInfo.extent.depth = 1;

    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format =
        gRenderImageFormat;

    imageInfo.tiling =
        VK_IMAGE_TILING_OPTIMAL;

    imageInfo.initialLayout =
        VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    imageInfo.samples =
        VK_SAMPLE_COUNT_1_BIT;

    imageInfo.sharingMode =
        VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(
            gDevice,
            &imageInfo,
            nullptr,
            &gRenderImage) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan render image\n";
        return false;
    }

    VkMemoryRequirements memoryRequirements{};

    vkGetImageMemoryRequirements(
        gDevice,
        gRenderImage,
        &memoryRequirements
    );

    VkMemoryAllocateInfo allocateInfo{};

    allocateInfo.sType =
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    allocateInfo.allocationSize =
        memoryRequirements.size;

    allocateInfo.memoryTypeIndex =
        FindMemoryType(
            memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

    if (vkAllocateMemory(
            gDevice,
            &allocateInfo,
            nullptr,
            &gRenderImageMemory) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to allocate Vulkan render image memory\n";

        vkDestroyImage(
            gDevice,
            gRenderImage,
            nullptr
        );

        gRenderImage =
            VK_NULL_HANDLE;

        return false;
    }

    if (vkBindImageMemory(
            gDevice,
            gRenderImage,
            gRenderImageMemory,
            0) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to bind Vulkan render image memory\n";

        vkFreeMemory(
            gDevice,
            gRenderImageMemory,
            nullptr
        );

        vkDestroyImage(
            gDevice,
            gRenderImage,
            nullptr
        );

        gRenderImageMemory =
            VK_NULL_HANDLE;

        gRenderImage =
            VK_NULL_HANDLE;

        return false;
    }

    VkImageViewCreateInfo viewInfo{};

    viewInfo.sType =
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    viewInfo.image =
        gRenderImage;

    viewInfo.viewType =
        VK_IMAGE_VIEW_TYPE_2D;

    viewInfo.format =
        gRenderImageFormat;

    viewInfo.components.r =
        VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.components.g =
        VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.components.b =
        VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.components.a =
        VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;

    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(
            gDevice,
            &viewInfo,
            nullptr,
            &gRenderImageView) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan render image view\n";

        vkFreeMemory(
            gDevice,
            gRenderImageMemory,
            nullptr
        );

        vkDestroyImage(
            gDevice,
            gRenderImage,
            nullptr
        );

        gRenderImageMemory =
            VK_NULL_HANDLE;

        gRenderImage =
            VK_NULL_HANDLE;

        return false;
    }

    gRenderImageExtent.width =
        static_cast<uint32_t>(width);

    gRenderImageExtent.height =
        static_cast<uint32_t>(height);

    return true;
}

void DestroyRenderImage()
{
    if (gRenderImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(
            gDevice,
            gRenderImageView,
            nullptr
        );

        gRenderImageView =
            VK_NULL_HANDLE;
    }

    if (gRenderImage != VK_NULL_HANDLE) {
        vkDestroyImage(
            gDevice,
            gRenderImage,
            nullptr
        );

        gRenderImage =
            VK_NULL_HANDLE;
    }

    if (gRenderImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(
            gDevice,
            gRenderImageMemory,
            nullptr
        );

        gRenderImageMemory =
            VK_NULL_HANDLE;
    }

    gRenderImageExtent = {};
	gRenderImageInitialised = false;
}

bool CreateLogicalDevice()
{
    const QueueFamilyIndices indices =
        FindQueueFamilies(gPhysicalDevice);

    if (!indices.IsComplete()) {
        std::cerr
            << "Selected GPU has incomplete queue families\n";
        return false;
    }

    const uint32_t graphicsFamily =
        indices.graphicsFamily.value();

    const uint32_t presentFamily =
        indices.presentFamily.value();

    std::vector<uint32_t> uniqueQueueFamilies;

    uniqueQueueFamilies.push_back(graphicsFamily);

    if (presentFamily != graphicsFamily) {
        uniqueQueueFamilies.push_back(presentFamily);
    }

    const float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};

        queueCreateInfo.sType =
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

        queueCreateInfo.queueFamilyIndex =
            queueFamily;

        queueCreateInfo.queueCount = 1;

        queueCreateInfo.pQueuePriorities =
            &queuePriority;

        queueCreateInfos.push_back(
            queueCreateInfo
        );
    }

    const char* requiredExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures deviceFeatures{};
	
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};

	dynamicRenderingFeatures.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

	dynamicRenderingFeatures.dynamicRendering =
		VK_TRUE;
	
    VkDeviceCreateInfo createInfo{};

    createInfo.sType =
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(
            queueCreateInfos.size()
        );

    createInfo.pQueueCreateInfos =
        queueCreateInfos.data();

    createInfo.pEnabledFeatures =
        &deviceFeatures;

	createInfo.pNext =
    &dynamicRenderingFeatures;

    createInfo.enabledExtensionCount = 1;

    createInfo.ppEnabledExtensionNames =
        requiredExtensions;

    if (vkCreateDevice(
            gPhysicalDevice,
            &createInfo,
            nullptr,
            &gDevice) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan logical device\n";
        return false;
    }

    vkGetDeviceQueue(
        gDevice,
        graphicsFamily,
        0,
        &gGraphicsQueue
    );

    vkGetDeviceQueue(
        gDevice,
        presentFamily,
        0,
        &gPresentQueue
    );

    std::cout
        << "Vulkan logical device created successfully\n";

    std::cout
        << "Graphics queue family: "
        << graphicsFamily
        << '\n';

    std::cout
        << "Present queue family: "
        << presentFamily
        << '\n';

    return true;
}

bool CreateSwapchain()
{
    const SwapchainSupportDetails support =
        QuerySwapchainSupport(gPhysicalDevice);

    if (support.formats.empty()) {
        std::cerr
            << "No Vulkan surface formats available\n";
        return false;
    }

    if (support.presentModes.empty()) {
        std::cerr
            << "No Vulkan present modes available\n";
        return false;
    }

    const VkSurfaceFormatKHR surfaceFormat =
        ChooseSwapchainSurfaceFormat(
            support.formats
        );

    const VkPresentModeKHR presentMode =
        ChooseSwapchainPresentMode(
            support.presentModes
        );

    const VkExtent2D extent =
        ChooseSwapchainExtent(
            support.capabilities
        );

    if ((support.capabilities.supportedUsageFlags &
         VK_IMAGE_USAGE_TRANSFER_DST_BIT) == 0)
    {
        std::cerr
            << "Swapchain images do not support transfer destination usage\n";
        return false;
    }

    uint32_t imageCount =
        support.capabilities.minImageCount + 1;

    if (support.capabilities.maxImageCount > 0 &&
        imageCount >
            support.capabilities.maxImageCount)
    {
        imageCount =
            support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};

    createInfo.sType =
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    createInfo.surface = gSurface;

    createInfo.minImageCount = imageCount;

    createInfo.imageFormat =
        surfaceFormat.format;

    createInfo.imageColorSpace =
        surfaceFormat.colorSpace;

    createInfo.imageExtent = extent;

    createInfo.imageArrayLayers = 1;

    createInfo.imageUsage =
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    const QueueFamilyIndices indices =
        FindQueueFamilies(gPhysicalDevice);

    const uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    if (indices.graphicsFamily !=
        indices.presentFamily)
    {
        createInfo.imageSharingMode =
            VK_SHARING_MODE_CONCURRENT;

        createInfo.queueFamilyIndexCount = 2;

        createInfo.pQueueFamilyIndices =
            queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode =
            VK_SHARING_MODE_EXCLUSIVE;

        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform =
        support.capabilities.currentTransform;

    createInfo.compositeAlpha =
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode =
        presentMode;

    createInfo.clipped = VK_TRUE;

    createInfo.oldSwapchain =
        VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(
            gDevice,
            &createInfo,
            nullptr,
            &gSwapchain) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan swapchain\n";
        return false;
    }

    uint32_t actualImageCount = 0;

    vkGetSwapchainImagesKHR(
        gDevice,
        gSwapchain,
        &actualImageCount,
        nullptr
    );

    gSwapchainImages.resize(
        actualImageCount
    );

    vkGetSwapchainImagesKHR(
        gDevice,
        gSwapchain,
        &actualImageCount,
        gSwapchainImages.data()
    );

    gSwapchainImageInitialised.assign(
        actualImageCount,
        false
    );

    gSwapchainImageFormat =
        surfaceFormat.format;

    gSwapchainExtent =
        extent;

    std::cout
        << "Vulkan swapchain created successfully\n";

    std::cout
        << "Swapchain images: "
        << actualImageCount
        << '\n';

    std::cout
        << "Swapchain extent: "
        << extent.width
        << "x"
        << extent.height
        << '\n';

    return true;
}

bool CreateSwapchainImageViews()
{
    gSwapchainImageViews.resize(
        gSwapchainImages.size()
    );

    for (std::size_t i = 0;
         i < gSwapchainImages.size();
         ++i)
    {
        VkImageViewCreateInfo createInfo{};

        createInfo.sType =
            VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        createInfo.image =
            gSwapchainImages[i];

        createInfo.viewType =
            VK_IMAGE_VIEW_TYPE_2D;

        createInfo.format =
            gSwapchainImageFormat;

        createInfo.components.r =
            VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.g =
            VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.b =
            VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.components.a =
            VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(
                gDevice,
                &createInfo,
                nullptr,
                &gSwapchainImageViews[i]) != VK_SUCCESS)
        {
            std::cerr
                << "Failed to create Vulkan swapchain image view\n";

            for (std::size_t j = 0; j < i; ++j) {
                vkDestroyImageView(
                    gDevice,
                    gSwapchainImageViews[j],
                    nullptr
                );
            }

            gSwapchainImageViews.clear();

            return false;
        }
    }

    std::cout
        << "Vulkan swapchain image views created successfully\n";

    return true;
}

bool CreateCommandResources()
{
    const QueueFamilyIndices indices =
        FindQueueFamilies(gPhysicalDevice);

    if (!indices.graphicsFamily.has_value()) {
        std::cerr
            << "No graphics queue family available\n";
        return false;
    }

    VkCommandPoolCreateInfo poolInfo{};

    poolInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

    poolInfo.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    poolInfo.queueFamilyIndex =
        indices.graphicsFamily.value();

    if (vkCreateCommandPool(
            gDevice,
            &poolInfo,
            nullptr,
            &gCommandPool) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create Vulkan command pool\n";
        return false;
    }

    VkCommandBufferAllocateInfo allocateInfo{};

    allocateInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

    allocateInfo.commandPool =
        gCommandPool;

    allocateInfo.level =
        VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    allocateInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(
            gDevice,
            &allocateInfo,
            &gCommandBuffer) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to allocate Vulkan command buffer\n";

        vkDestroyCommandPool(
            gDevice,
            gCommandPool,
            nullptr
        );

        gCommandPool = VK_NULL_HANDLE;

        return false;
    }

    std::cout
        << "Vulkan command resources created successfully\n";

    return true;
}

bool CreateSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};

    semaphoreInfo.sType =
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};

    fenceInfo.sType =
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    fenceInfo.flags =
        VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(
            gDevice,
            &semaphoreInfo,
            nullptr,
            &gImageAvailableSemaphore) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create image-available semaphore\n";
        return false;
    }

    if (vkCreateSemaphore(
            gDevice,
            &semaphoreInfo,
            nullptr,
            &gRenderFinishedSemaphore) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create render-finished semaphore\n";

        vkDestroySemaphore(
            gDevice,
            gImageAvailableSemaphore,
            nullptr
        );

        gImageAvailableSemaphore = VK_NULL_HANDLE;

        return false;
    }

    if (vkCreateFence(
            gDevice,
            &fenceInfo,
            nullptr,
            &gRenderFence) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to create render fence\n";

        vkDestroySemaphore(
            gDevice,
            gRenderFinishedSemaphore,
            nullptr
        );

        vkDestroySemaphore(
            gDevice,
            gImageAvailableSemaphore,
            nullptr
        );

        gRenderFinishedSemaphore = VK_NULL_HANDLE;
        gImageAvailableSemaphore = VK_NULL_HANDLE;

        return false;
    }

    std::cout
        << "Vulkan synchronization objects created successfully\n";

    return true;
}

bool RecreateSwapchain()
{
    int width = 0;
    int height = 0;

       OpenLRR::Platform::GetFramebufferSize(
        &width,
        &height
    );

    if (width <= 0 || height <= 0) {
        return true;
    }

    if (gSwapchainExtent.width ==
            static_cast<uint32_t>(width) &&
        gSwapchainExtent.height ==
            static_cast<uint32_t>(height))
    {
        return true;
    }

    vkDeviceWaitIdle(gDevice);

    for (VkImageView imageView :
         gSwapchainImageViews)
    {
        vkDestroyImageView(
            gDevice,
            imageView,
            nullptr
        );
    }

    gSwapchainImageViews.clear();

    if (gSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            gDevice,
            gSwapchain,
            nullptr
        );

        gSwapchain = VK_NULL_HANDLE;
    }

    gSwapchainImages.clear();
    gSwapchainImageInitialised.clear();

    gSwapchainImageFormat =
        VK_FORMAT_UNDEFINED;

    gSwapchainExtent = {};

    if (!CreateSwapchain()) {
        std::cerr
            << "Failed to recreate Vulkan swapchain\n";
        return false;
    }
	
	    if (!CreateSwapchainImageViews()) {
        std::cerr
            << "Failed to recreate Vulkan swapchain image views\n";
        return false;
    }

       std::cout
    << "Vulkan swapchain recreated\n";

return true;
}

bool BeginFrameInternal(
    const OpenLRR::Settings::Resolution& renderResolution)
{
    gFrameInProgress = false;

         const auto now =
        std::chrono::steady_clock::now();

    if (OpenLRR::Platform::WasFramebufferResized()) {
        gSwapchainResizePending = true;

        gLastFramebufferResizeEvent =
            now;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;

    OpenLRR::Platform::GetFramebufferSize(
        &framebufferWidth,
        &framebufferHeight
    );

    if (framebufferWidth <= 0 ||
        framebufferHeight <= 0)
    {
        return true;
    }

	gCurrentPresentation =
    OpenLRR::Renderer::CalculatePresentationViewport(
        framebufferWidth,
        framebufferHeight,
        renderResolution.width,
        renderResolution.height
    );

if (gCurrentPresentation.width <= 0 ||
    gCurrentPresentation.height <= 0)
{
    return true;
}

    if (gSwapchainResizePending) {
        if (now - gLastFramebufferResizeEvent <
            kResizeDebounceInterval)
        {
            return true;
        }

        const bool extentAlreadyMatches =
            gSwapchainExtent.width ==
                static_cast<uint32_t>(framebufferWidth) &&
            gSwapchainExtent.height ==
                static_cast<uint32_t>(framebufferHeight);

        if (!extentAlreadyMatches) {
            if (!RecreateSwapchain()) {
                return false;
            }
        }

        gSwapchainResizePending = false;
    }   

    vkWaitForFences(
        gDevice,
        1,
        &gRenderFence,
        VK_TRUE,
        UINT64_MAX
    );

    const VkResult acquireResult =
        vkAcquireNextImageKHR(
            gDevice,
            gSwapchain,
            UINT64_MAX,
            gImageAvailableSemaphore,
            VK_NULL_HANDLE,
            &gCurrentImageIndex
        );

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
    gSwapchainResizePending = true;

    gLastFramebufferResizeEvent =
        std::chrono::steady_clock::now();

    return true;
	}

    if (acquireResult != VK_SUCCESS &&
        acquireResult != VK_SUBOPTIMAL_KHR)
    {
        std::cerr
            << "Failed to acquire Vulkan swapchain image\n";
        return false;
    }

    vkResetFences(
        gDevice,
        1,
        &gRenderFence
    );

    vkResetCommandBuffer(
        gCommandBuffer,
        0
    );

    VkCommandBufferBeginInfo beginInfo{};

    beginInfo.sType =
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    beginInfo.flags =
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(
            gCommandBuffer,
            &beginInfo) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to begin Vulkan command buffer\n";
        return false;
    }

	VkImageMemoryBarrier renderImageBarrier{};

	renderImageBarrier.sType =
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	renderImageBarrier.oldLayout =
		gRenderImageInitialised
			? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			: VK_IMAGE_LAYOUT_UNDEFINED;

	renderImageBarrier.newLayout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	renderImageBarrier.srcQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	renderImageBarrier.dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	renderImageBarrier.image =
		gRenderImage;

	renderImageBarrier.subresourceRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	renderImageBarrier.subresourceRange.baseMipLevel = 0;
	renderImageBarrier.subresourceRange.levelCount = 1;
	renderImageBarrier.subresourceRange.baseArrayLayer = 0;
	renderImageBarrier.subresourceRange.layerCount = 1;

	renderImageBarrier.srcAccessMask =
		gRenderImageInitialised
			? VK_ACCESS_TRANSFER_READ_BIT
			: 0;

	renderImageBarrier.dstAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vkCmdPipelineBarrier(
		gCommandBuffer,
		gRenderImageInitialised
			? VK_PIPELINE_STAGE_TRANSFER_BIT
			: VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&renderImageBarrier
	);

    VkClearValue backgroundClear{};

    backgroundClear.color.float32[0] = 0.0f;
	backgroundClear.color.float32[1] = 0.0f;
	backgroundClear.color.float32[2] = 0.0f;
	backgroundClear.color.float32[3] = 1.0f;

    VkRenderingAttachmentInfo colorAttachment{};

    colorAttachment.sType =
        VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    colorAttachment.imageView =
		gRenderImageView;

    colorAttachment.imageLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    colorAttachment.loadOp =
        VK_ATTACHMENT_LOAD_OP_CLEAR;

    colorAttachment.storeOp =
        VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.clearValue =
        backgroundClear;

    VkRenderingInfo renderingInfo{};

    renderingInfo.sType =
        VK_STRUCTURE_TYPE_RENDERING_INFO;

    renderingInfo.renderArea.offset = {
		0,
		0
	};

	renderingInfo.renderArea.extent =
		gRenderImageExtent;

    renderingInfo.layerCount = 1;

    renderingInfo.colorAttachmentCount = 1;

    renderingInfo.pColorAttachments =
        &colorAttachment;

    vkCmdBeginRendering(
		gCommandBuffer,
		&renderingInfo
	);

	gFrameInProgress = true;

	return true;
	}

	bool EndFrameInternal()
	{
		if (!gFrameInProgress) {
			return true;
		}

		vkCmdEndRendering(
			gCommandBuffer
		);
    VkImageMemoryBarrier toTransferSourceBarrier{};

	toTransferSourceBarrier.sType =
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	toTransferSourceBarrier.oldLayout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	toTransferSourceBarrier.newLayout =
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

	toTransferSourceBarrier.srcQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	toTransferSourceBarrier.dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	toTransferSourceBarrier.image =
		gRenderImage;

	toTransferSourceBarrier.subresourceRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	toTransferSourceBarrier.subresourceRange.baseMipLevel = 0;
	toTransferSourceBarrier.subresourceRange.levelCount = 1;
	toTransferSourceBarrier.subresourceRange.baseArrayLayer = 0;
	toTransferSourceBarrier.subresourceRange.layerCount = 1;

	toTransferSourceBarrier.srcAccessMask =
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	toTransferSourceBarrier.dstAccessMask =
		VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(
		gCommandBuffer,
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&toTransferSourceBarrier
	);

	gRenderImageInitialised = true;
	
	VkImageMemoryBarrier swapchainToTransferDst{};

	swapchainToTransferDst.sType =
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	swapchainToTransferDst.oldLayout =
		gSwapchainImageInitialised[gCurrentImageIndex]
			? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			: VK_IMAGE_LAYOUT_UNDEFINED;

	swapchainToTransferDst.newLayout =
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	swapchainToTransferDst.srcQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	swapchainToTransferDst.dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	swapchainToTransferDst.image =
		gSwapchainImages[gCurrentImageIndex];

	swapchainToTransferDst.subresourceRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	swapchainToTransferDst.subresourceRange.baseMipLevel = 0;
	swapchainToTransferDst.subresourceRange.levelCount = 1;
	swapchainToTransferDst.subresourceRange.baseArrayLayer = 0;
	swapchainToTransferDst.subresourceRange.layerCount = 1;

	swapchainToTransferDst.srcAccessMask = 0;

	swapchainToTransferDst.dstAccessMask =
		VK_ACCESS_TRANSFER_WRITE_BIT;

	vkCmdPipelineBarrier(
		gCommandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&swapchainToTransferDst
	);
	
	VkClearColorValue blackClear{};

	blackClear.float32[0] = 0.0f;
	blackClear.float32[1] = 0.0f;
	blackClear.float32[2] = 0.0f;
	blackClear.float32[3] = 1.0f;

	VkImageSubresourceRange clearRange{};

	clearRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	clearRange.baseMipLevel = 0;
	clearRange.levelCount = 1;
	clearRange.baseArrayLayer = 0;
	clearRange.layerCount = 1;

	vkCmdClearColorImage(
		gCommandBuffer,
		gSwapchainImages[gCurrentImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&blackClear,
		1,
		&clearRange
	);
	
	VkImageBlit blitRegion{};

	blitRegion.srcSubresource.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	blitRegion.srcSubresource.mipLevel = 0;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;

	blitRegion.srcOffsets[0] = {
		0,
		0,
		0
	};

	blitRegion.srcOffsets[1] = {
		static_cast<int32_t>(gRenderImageExtent.width),
		static_cast<int32_t>(gRenderImageExtent.height),
		1
	};

	blitRegion.dstSubresource.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	blitRegion.dstSubresource.mipLevel = 0;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;

	blitRegion.dstOffsets[0] = {
		static_cast<int32_t>(gCurrentPresentation.x),
		static_cast<int32_t>(gCurrentPresentation.y),
		0
	};

	blitRegion.dstOffsets[1] = {
		static_cast<int32_t>(
			gCurrentPresentation.x + gCurrentPresentation.width
		),
		static_cast<int32_t>(
			gCurrentPresentation.y + gCurrentPresentation.height
		),
		1
	};

	vkCmdBlitImage(
		gCommandBuffer,
		gRenderImage,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		gSwapchainImages[gCurrentImageIndex],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&blitRegion,
		VK_FILTER_LINEAR
	);
	
	VkImageMemoryBarrier swapchainToPresent{};

	swapchainToPresent.sType =
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	swapchainToPresent.oldLayout =
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	swapchainToPresent.newLayout =
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	swapchainToPresent.srcQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	swapchainToPresent.dstQueueFamilyIndex =
		VK_QUEUE_FAMILY_IGNORED;

	swapchainToPresent.image =
		gSwapchainImages[gCurrentImageIndex];

	swapchainToPresent.subresourceRange.aspectMask =
		VK_IMAGE_ASPECT_COLOR_BIT;

	swapchainToPresent.subresourceRange.baseMipLevel = 0;
	swapchainToPresent.subresourceRange.levelCount = 1;
	swapchainToPresent.subresourceRange.baseArrayLayer = 0;
	swapchainToPresent.subresourceRange.layerCount = 1;

	swapchainToPresent.srcAccessMask =
		VK_ACCESS_TRANSFER_WRITE_BIT;

	swapchainToPresent.dstAccessMask = 0;

	vkCmdPipelineBarrier(
		gCommandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&swapchainToPresent
	);
	
    if (vkEndCommandBuffer(
            gCommandBuffer) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to record Vulkan command buffer\n";
        return false;
    }

    const VkPipelineStageFlags waitStage =
		VK_PIPELINE_STAGE_TRANSFER_BIT;

    VkSubmitInfo submitInfo{};

    submitInfo.sType =
        VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = 1;

    submitInfo.pWaitSemaphores =
        &gImageAvailableSemaphore;

    submitInfo.pWaitDstStageMask =
        &waitStage;

    submitInfo.commandBufferCount = 1;

    submitInfo.pCommandBuffers =
        &gCommandBuffer;

    submitInfo.signalSemaphoreCount = 1;

    submitInfo.pSignalSemaphores =
        &gRenderFinishedSemaphore;

    if (vkQueueSubmit(
            gGraphicsQueue,
            1,
            &submitInfo,
            gRenderFence) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to submit Vulkan frame\n";
        return false;
    }

    gSwapchainImageInitialised[gCurrentImageIndex] = true;

    VkPresentInfoKHR presentInfo{};

    presentInfo.sType =
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pWaitSemaphores =
        &gRenderFinishedSemaphore;

    presentInfo.swapchainCount = 1;

    presentInfo.pSwapchains =
        &gSwapchain;

    presentInfo.pImageIndices =
        &gCurrentImageIndex;

    const VkResult presentResult =
        vkQueuePresentKHR(
            gPresentQueue,
            &presentInfo
        );

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR ||
		presentResult == VK_SUBOPTIMAL_KHR)
	{
		gSwapchainResizePending = true;

		gLastFramebufferResizeEvent =
			std::chrono::steady_clock::now();

    return true;
	}

    if (presentResult != VK_SUCCESS) {
		std::cerr
			<< "Failed to present Vulkan frame\n";

		gFrameInProgress = false;
		return false;
	}

	gFrameInProgress = false;

	return true;
	}

}

namespace OpenLRR::Renderer
{
    bool InitialiseVulkan(
		const OpenLRR::Settings::Resolution& renderResolution)
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

        if (!SelectPhysicalDevice()) {
            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;

            return false;
        }

        if (!CreateLogicalDevice()) {
            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;

            gPhysicalDevice = VK_NULL_HANDLE;

            return false;
        }

		if (!CreateRenderImage(
				renderResolution.width,
				renderResolution.height))
		{
			std::cerr
				<< "Failed to create canonical render image\n";

			vkDestroyDevice(
				gDevice,
				nullptr
			);

			gDevice = VK_NULL_HANDLE;
			gGraphicsQueue = VK_NULL_HANDLE;
			gPresentQueue = VK_NULL_HANDLE;

			vkDestroySurfaceKHR(
				gInstance,
				gSurface,
				nullptr
			);

			gSurface = VK_NULL_HANDLE;

			vkDestroyInstance(
				gInstance,
				nullptr
			);

			gInstance = VK_NULL_HANDLE;
			gPhysicalDevice = VK_NULL_HANDLE;

			return false;
		}

        if (!CreateSwapchain()) {
			DestroyRenderImage();
			
            vkDestroyDevice(
                gDevice,
                nullptr
            );

            gDevice = VK_NULL_HANDLE;
            gGraphicsQueue = VK_NULL_HANDLE;
            gPresentQueue = VK_NULL_HANDLE;

            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;
            gPhysicalDevice = VK_NULL_HANDLE;

            return false;
        }

        if (!CreateSwapchainImageViews()) {
            vkDestroySwapchainKHR(
                gDevice,
                gSwapchain,
                nullptr
            );

            gSwapchain = VK_NULL_HANDLE;
            gSwapchainImages.clear();
            gSwapchainImageInitialised.clear();

            vkDestroyDevice(
                gDevice,
                nullptr
            );

            gDevice = VK_NULL_HANDLE;
            gGraphicsQueue = VK_NULL_HANDLE;
            gPresentQueue = VK_NULL_HANDLE;

            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;
            gPhysicalDevice = VK_NULL_HANDLE;

            return false;
        }

        if (!CreateCommandResources()) {
            vkDestroySwapchainKHR(
                gDevice,
                gSwapchain,
                nullptr
            );

            gSwapchain = VK_NULL_HANDLE;
            gSwapchainImages.clear();
            gSwapchainImageInitialised.clear();

            vkDestroyDevice(
                gDevice,
                nullptr
            );

            gDevice = VK_NULL_HANDLE;
            gGraphicsQueue = VK_NULL_HANDLE;
            gPresentQueue = VK_NULL_HANDLE;

            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;
            gPhysicalDevice = VK_NULL_HANDLE;

            return false;
        }

        if (!CreateSyncObjects()) {
            vkDestroyCommandPool(
                gDevice,
                gCommandPool,
                nullptr
            );

            gCommandPool = VK_NULL_HANDLE;
            gCommandBuffer = VK_NULL_HANDLE;

            vkDestroySwapchainKHR(
                gDevice,
                gSwapchain,
                nullptr
            );

            gSwapchain = VK_NULL_HANDLE;
            gSwapchainImages.clear();
            gSwapchainImageInitialised.clear();

            vkDestroyDevice(
                gDevice,
                nullptr
            );

            gDevice = VK_NULL_HANDLE;
            gGraphicsQueue = VK_NULL_HANDLE;
            gPresentQueue = VK_NULL_HANDLE;

            vkDestroySurfaceKHR(
                gInstance,
                gSurface,
                nullptr
            );

            gSurface = VK_NULL_HANDLE;

            vkDestroyInstance(
                gInstance,
                nullptr
            );

            gInstance = VK_NULL_HANDLE;
            gPhysicalDevice = VK_NULL_HANDLE;

            return false;
        }

        std::cout
            << "Vulkan instance created successfully\n";

        std::cout
            << "Vulkan window surface created successfully\n";

        return true;
    }

    void SetClearColor(
        float red,
        float green,
        float blue,
        float alpha)
    {
        gClearColor[0] = red;
        gClearColor[1] = green;
        gClearColor[2] = blue;
        gClearColor[3] = alpha;
    }

	    int GetRenderWidth()
    {
        return static_cast<int>(
            gRenderImageExtent.width
        );
    }

    int GetRenderHeight()
    {
        return static_cast<int>(
            gRenderImageExtent.height
        );
    }
	
	PresentationViewport GetPresentationViewport()
    {
        return gCurrentPresentation;
    }
	
    void DrawFilledRect(
        int x,
        int y,
        int width,
        int height,
        float red,
        float green,
        float blue,
        float alpha)
    {
        if (width <= 0 || height <= 0) {
            return;
        }

        VkClearAttachment clearAttachment{};

        clearAttachment.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        clearAttachment.colorAttachment = 0;

        clearAttachment.clearValue.color.float32[0] = red;
        clearAttachment.clearValue.color.float32[1] = green;
        clearAttachment.clearValue.color.float32[2] = blue;
        clearAttachment.clearValue.color.float32[3] = alpha;

        VkClearRect clearRect{};

        clearRect.rect.offset = {
            x,
            y
        };

        clearRect.rect.extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        vkCmdClearAttachments(
            gCommandBuffer,
            1,
            &clearAttachment,
            1,
            &clearRect
        );
    }

    bool BeginFrame(
		const OpenLRR::Settings::Resolution& renderResolution)
	{
		return BeginFrameInternal(renderResolution);
	}

	bool EndFrame()
	{
		return EndFrameInternal();
	}

	bool IsFrameInProgress()
	{
		return gFrameInProgress;
	}

        void ShutdownVulkan()
    {
	            if (gDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(gDevice);
        }

        if (gRenderFence != VK_NULL_HANDLE) {
            vkDestroyFence(
                gDevice,
                gRenderFence,
                nullptr
            );

            gRenderFence = VK_NULL_HANDLE;
        }

        if (gRenderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(
                gDevice,
                gRenderFinishedSemaphore,
                nullptr
            );

            gRenderFinishedSemaphore = VK_NULL_HANDLE;
        }

        if (gImageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(
                gDevice,
                gImageAvailableSemaphore,
                nullptr
            );

            gImageAvailableSemaphore = VK_NULL_HANDLE;
        }

        if (gCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(
                gDevice,
                gCommandPool,
                nullptr
            );

            gCommandPool = VK_NULL_HANDLE;
            gCommandBuffer = VK_NULL_HANDLE;
        }
		
		        if (gDevice != VK_NULL_HANDLE) {
            for (VkImageView imageView :
                 gSwapchainImageViews)
            {
                vkDestroyImageView(
                    gDevice,
                    imageView,
                    nullptr
                );
            }

            gSwapchainImageViews.clear();
        }
		
		if (gSwapchain != VK_NULL_HANDLE &&
            gDevice != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(
                gDevice,
                gSwapchain,
                nullptr
            );

            gSwapchain = VK_NULL_HANDLE;

            gSwapchainImages.clear();
			gSwapchainImageInitialised.clear();

            gSwapchainImageFormat =
                VK_FORMAT_UNDEFINED;

            gSwapchainExtent = {};
        }
	
		if (gDevice != VK_NULL_HANDLE) {
			DestroyRenderImage();
		}
	
        if (gDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(
                gDevice,
                nullptr
            );

            gDevice = VK_NULL_HANDLE;
            gGraphicsQueue = VK_NULL_HANDLE;
            gPresentQueue = VK_NULL_HANDLE;
        }

        gPhysicalDevice = VK_NULL_HANDLE;

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
