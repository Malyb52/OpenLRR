#include "renderer/VulkanRenderer.hpp"
#include "platform/Platform.hpp"

#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <algorithm>
#include <chrono>
#include <limits>

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

bool RenderClearFrame()
{
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

    uint32_t imageIndex = 0;

    const VkResult acquireResult =
        vkAcquireNextImageKHR(
            gDevice,
            gSwapchain,
            UINT64_MAX,
            gImageAvailableSemaphore,
            VK_NULL_HANDLE,
            &imageIndex
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

        VkImageMemoryBarrier toColorAttachmentBarrier{};

    toColorAttachmentBarrier.sType =
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    toColorAttachmentBarrier.oldLayout =
        gSwapchainImageInitialised[imageIndex]
            ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            : VK_IMAGE_LAYOUT_UNDEFINED;

    toColorAttachmentBarrier.newLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    toColorAttachmentBarrier.srcQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;

    toColorAttachmentBarrier.dstQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;

    toColorAttachmentBarrier.image =
        gSwapchainImages[imageIndex];

    toColorAttachmentBarrier.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;

    toColorAttachmentBarrier.subresourceRange.baseMipLevel = 0;
    toColorAttachmentBarrier.subresourceRange.levelCount = 1;
    toColorAttachmentBarrier.subresourceRange.baseArrayLayer = 0;
    toColorAttachmentBarrier.subresourceRange.layerCount = 1;

    toColorAttachmentBarrier.srcAccessMask = 0;

    toColorAttachmentBarrier.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(
        gCommandBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &toColorAttachmentBarrier
    );

    VkClearValue backgroundClear{};

    backgroundClear.color.float32[0] = 0.04f;
    backgroundClear.color.float32[1] = 0.04f;
    backgroundClear.color.float32[2] = 0.04f;
    backgroundClear.color.float32[3] = 1.0f;

    VkRenderingAttachmentInfo colorAttachment{};

    colorAttachment.sType =
        VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    colorAttachment.imageView =
        gSwapchainImageViews[imageIndex];

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
        gSwapchainExtent;

    renderingInfo.layerCount = 1;

    renderingInfo.colorAttachmentCount = 1;

    renderingInfo.pColorAttachments =
        &colorAttachment;

    vkCmdBeginRendering(
        gCommandBuffer,
        &renderingInfo
    );

    constexpr uint32_t tileSize = 128;

    const VkClearColorValue tileColors[] = {
        {{ 0.10f, 0.20f, 0.55f, 1.0f }},
        {{ 0.10f, 0.55f, 0.28f, 1.0f }},
        {{ 0.55f, 0.18f, 0.16f, 1.0f }},
        {{ 0.55f, 0.45f, 0.10f, 1.0f }}
    };

    for (uint32_t y = 0;
         y < gSwapchainExtent.height;
         y += tileSize)
    {
        for (uint32_t x = 0;
             x < gSwapchainExtent.width;
             x += tileSize)
        {
            const uint32_t tileX =
                x / tileSize;

            const uint32_t tileY =
                y / tileSize;

            const uint32_t colorIndex =
                (tileX + (tileY * 3)) % 4;

            VkClearAttachment clearAttachment{};

            clearAttachment.aspectMask =
                VK_IMAGE_ASPECT_COLOR_BIT;

            clearAttachment.colorAttachment = 0;

            clearAttachment.clearValue.color =
                tileColors[colorIndex];

            VkClearRect clearRect{};

            clearRect.rect.offset = {
                static_cast<int32_t>(x),
                static_cast<int32_t>(y)
            };

            clearRect.rect.extent.width =
                std::min(
                    tileSize,
                    gSwapchainExtent.width - x
                );

            clearRect.rect.extent.height =
                std::min(
                    tileSize,
                    gSwapchainExtent.height - y
                );

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
    }

    vkCmdEndRendering(
        gCommandBuffer
    );

    VkImageMemoryBarrier toPresentBarrier{};

    toPresentBarrier.sType =
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    toPresentBarrier.oldLayout =
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    toPresentBarrier.newLayout =
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    toPresentBarrier.srcQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;

    toPresentBarrier.dstQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;

    toPresentBarrier.image =
        gSwapchainImages[imageIndex];

    toPresentBarrier.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;

    toPresentBarrier.subresourceRange.baseMipLevel = 0;
    toPresentBarrier.subresourceRange.levelCount = 1;
    toPresentBarrier.subresourceRange.baseArrayLayer = 0;
    toPresentBarrier.subresourceRange.layerCount = 1;

    toPresentBarrier.srcAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    toPresentBarrier.dstAccessMask = 0;

    vkCmdPipelineBarrier(
        gCommandBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &toPresentBarrier
    );


    if (vkEndCommandBuffer(
            gCommandBuffer) != VK_SUCCESS)
    {
        std::cerr
            << "Failed to record Vulkan command buffer\n";
        return false;
    }

    const VkPipelineStageFlags waitStage =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

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

    gSwapchainImageInitialised[imageIndex] = true;

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
        &imageIndex;

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
        return false;
    }

    return true;
}

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

        if (!CreateSwapchain()) {
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

    bool RenderFrame()
    {
        return RenderClearFrame();
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
