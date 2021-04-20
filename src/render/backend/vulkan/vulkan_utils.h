#pragma once

#include "foundation/log/log_system.h"
#include "render/backend/vulkan/vulkan_config.h"

#include <GLFW/glfw3.h>
#include <vk_value_serialization.hpp>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR        capabilities {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

#define VK_TO_STRING(VKTYPE, VALUE) VulkanUtils::toString<VKTYPE>(#VKTYPE, VALUE)

class VulkanUtils {
public:
    static bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerPropertices : availableLayers)
            {
                if (strcmp(layerName, layerPropertices.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
                return false;
        }

        return true;
    }

    static std::vector<const char*> getRequiredExtensions()
    {
        uint32_t     glfwExtensionCount = 0;
        const char** glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (VulkanConfig::gEnableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void*                                       pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance                                instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                 const VkAllocationCallbacks*              pAllocator,
                                                 VkDebugUtilsMessengerEXT*                 pDebugMessenger)
    {
        const auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance                   instance,
                                              VkDebugUtilsMessengerEXT     debugMessenger,
                                              const VkAllocationCallbacks* pAllocator)
    {
        const auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMEssengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo       = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static bool checkDeviceExtensionSupported(VkPhysicalDevice physicalDevice)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(gDeviceExtensions.begin(), gDeviceExtensions.end());
        for (const auto& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    // Queue Family Utils
    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        QueueFamilyIndices indices;

        int index = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = index;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = index;
            }

            if (indices.isComplete())
                break;

            index++;
        }

        return indices;
    }

    static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        const bool isExtensionSupported = VulkanUtils::checkDeviceExtensionSupported(device);
        bool       isSwapChainAdequate  = false;
        if (isExtensionSupported)
        {
            const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
            isSwapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        const QueueFamilyIndices indices = VulkanUtils::findQueueFamilies(device, surface);

        VkPhysicalDeviceFeatures supportedFeatures {};
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && isExtensionSupported && isSwapChainAdequate &&
               supportedFeatures.samplerAnisotropy;
    }

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
    {
        if (capabilities.currentExtent.width != UINT32_MAX)
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

            actualExtent.width  = std::max(capabilities.minImageExtent.width,
                                          std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height,
                                           std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    static std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            LOG_FATAL("Failed to open file!");
        }

        const size_t      fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    template<typename VKTYPE>
    static std::string toString(std::string_view vkType, VKTYPE vkValue)
    {
        std::string str;

        bool parse = vk_serialize(vkType, static_cast<uint32_t>(vkValue), &str);
        if (parse == false)
        {
            LOG_ERROR("Parse Failed");
            return "";
        }

        return str;
    }

    static void dumpQueueFamilyInfo(VkPhysicalDevice physicalDevice)
    {
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        LOG_INFO("Queue Family Count: {}", queueFamilyCount);

        for (const auto& property : queueFamilies)
        {
            LOG_INFO("  Queue Count: {:2}, Queue Flags: {}",
                     property.queueCount,
                     VK_TO_STRING(VkQueueFlags, property.queueFlags));
        }
    }

    static void dumpExtensionInfo()
    {
        uint32_t vkExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
        std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());

        LOG_INFO("Available Extensions: {}", vkExtensionCount);
        for (const auto& vkExtension : vkExtensions)
        {
            LOG_INFO("  {}", vkExtension.extensionName);
        }
    }

    static void dumpPhysicalDeviceProperties(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceProperties properties;

        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        LOG_INFO("Physical Device Properties:");
        LOG_INFO("  {:20}{}", "API Veresion:", properties.apiVersion);
        LOG_INFO("  {:20}{}", "Driver Version:", properties.driverVersion);
        LOG_INFO("  {:20}{}", "Vendor ID:", properties.vendorID);
        LOG_INFO("  {:20}{}", "Device ID:", properties.deviceID);
        LOG_INFO("  {:20}{}", "Device Type:", VK_TO_STRING(VkPhysicalDeviceType, properties.deviceType));
        LOG_INFO("  {:20}{}", "Device Name:", properties.deviceName);
        LOG_INFO("  {:20}", "Device Limits:");

        const VkPhysicalDeviceLimits& deviceLimits = properties.limits;
        LOG_INFO("    {:32}{}", "Max Image Dimension 1D:", deviceLimits.maxImageDimension1D);
        LOG_INFO("    {:32}{}", "Max Image Dimension 2D:", deviceLimits.maxImageDimension2D);
        LOG_INFO("    {:32}{}", "Max Image Dimension 3D:", deviceLimits.maxImageDimension3D);
        LOG_INFO("    {:32}{}", "Max Image Dimension Cube:", deviceLimits.maxImageDimensionCube);
        LOG_INFO("    {:32}{}", "Max ImageArray Layers:", deviceLimits.maxImageArrayLayers);
        LOG_INFO("    {:32}{}", "Max TexelBuffer Elements:", deviceLimits.maxTexelBufferElements);
        LOG_INFO("    {:32}{}", "Max UniformBuffer Range:", deviceLimits.maxUniformBufferRange);
        LOG_INFO("    {:32}{}", "Max StorageBuffer Range:", deviceLimits.maxStorageBufferRange);
        LOG_INFO("    {:32}{}", "Max PushConstants Size:", deviceLimits.maxPushConstantsSize);
        LOG_INFO("    {:32}{}", "Max MemoryAllocation Count:", deviceLimits.maxMemoryAllocationCount);
        LOG_INFO("    {:32}{}", "Max SamplerAllocation Count:", deviceLimits.maxSamplerAllocationCount);
        LOG_INFO("    {:32}{}", "Max VertexInputAttributes:", deviceLimits.maxVertexInputAttributes);
        LOG_INFO("    {:32}{}", "Max VertexInputBindings:", deviceLimits.maxVertexInputBindings);
        LOG_INFO("    {:32}{}", "Max Framebuffer Width:", deviceLimits.maxFramebufferWidth);
        LOG_INFO("    {:32}{}", "Max Framebuffer Height:", deviceLimits.maxFramebufferHeight);
        LOG_INFO("    {:32}{}", "Max Framebuffer Layers:", deviceLimits.maxFramebufferLayers);
        LOG_INFO("    {:32}{}", "Max Viewports:", deviceLimits.maxViewports);
        LOG_INFO("    {:32}{}, {}",
                 "Max ViewportDimensions:",
                 deviceLimits.maxViewportDimensions[0],
                 deviceLimits.maxViewportDimensions[1]);
        LOG_INFO("    {:32}{}", "Max Clip Distances:", deviceLimits.maxClipDistances);
        LOG_INFO("    {:32}{}", "Max Cull Distances:", deviceLimits.maxCullDistances);
    }

    static void dumpSwapChainDetails(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
    {
        const SwapChainSupportDetails& details = querySwapChainSupport(physicalDevice, surface);

        LOG_INFO("SwapChain Details:");
        const VkSurfaceCapabilitiesKHR& capabilities = details.capabilities;
        LOG_INFO("  SwapChain Capabilities:");
        LOG_INFO("    {:32}{}", "Min Image Count:", capabilities.minImageCount);
        LOG_INFO("    {:32}{}", "Max Image Count:", capabilities.maxImageCount);
        LOG_INFO(
            "    {:32}{}, {}", "Current Extent:", capabilities.currentExtent.width, capabilities.currentExtent.height);
        LOG_INFO("    {:32}{}, {}",
                 "Min Image Extent:",
                 capabilities.minImageExtent.width,
                 capabilities.minImageExtent.height);
        LOG_INFO("    {:32}{}, {}",
                 "Max Image Extent:",
                 capabilities.maxImageExtent.width,
                 capabilities.maxImageExtent.height);
        LOG_INFO("    {:32}{}", "Max ImageArray Layers:", capabilities.maxImageArrayLayers);
        LOG_INFO("    {:32}{}",
                 "Supported Transforms:",
                 VK_TO_STRING(VkSurfaceTransformFlagsKHR, capabilities.supportedTransforms));
        LOG_INFO("    {:32}{}",
                 "Current Transform:",
                 VK_TO_STRING(VkSurfaceTransformFlagBitsKHR, capabilities.currentTransform));
        LOG_INFO("    {:32}{}",
                 "Supported Composite Alpha:",
                 VK_TO_STRING(VkCompositeAlphaFlagsKHR, capabilities.supportedTransforms));
        LOG_INFO(
            "    {:32}{}", "Supported Usage Flags:", VK_TO_STRING(VkImageUsageFlags, capabilities.supportedUsageFlags));

        LOG_INFO("  SwapChain Formats:")
        for (const VkSurfaceFormatKHR& format : details.formats)
        {
            LOG_INFO("    {:16}{:32} {:16}{}",
                     "Format:",
                     VK_TO_STRING(VkFormat, format.format),
                     "ColorSpace:",
                     VK_TO_STRING(VkColorSpaceKHR, format.colorSpace));
        }

        LOG_INFO("  SwapChain PresentModes:");
        for (const VkPresentModeKHR presentMode : details.presentModes)
        {
            LOG_INFO("    {:16}{}", "Present Mode:", VK_TO_STRING(VkPresentModeKHR, presentMode));
        }
    }
};
