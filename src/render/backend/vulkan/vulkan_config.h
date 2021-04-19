#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#define VK_VALUE_SERIALIZATION_CONFIG_MAIN

#include <vulkan/vulkan.h>

#include <vector>

namespace VulkanConfig
{
#ifdef NDEBUG
    const bool gEnableValidationLayers = false;
#else
    const bool gEnableValidationLayers = true;
#endif

    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    const std::vector<const char*> gValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> gDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

};

using namespace VulkanConfig;