#define GLFW_INCLUDE_VULKAN

#include "vulkan_app.h"
#include "vulkan_config.h"
#include "vulkan_utils.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <optional>
#include <algorithm>
#include <set>


void VulkanApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window_ = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

}

void VulkanApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
}

void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
    }
}

void VulkanApp::cleanup()
{
    vkDestroySwapchainKHR(device_, swapChain_, nullptr);

    vkDestroyDevice(device_, nullptr);

    vkDestroySurfaceKHR(instance_, surface_, nullptr);

    if (gEnableValidationLayers)
    {
        VulkanUtils::DestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);

    glfwDestroyWindow(window_);
    glfwTerminate();
}

void VulkanApp::createInstance()
{
    if (gEnableValidationLayers && !VulkanUtils::checkValidationLayerSupport(gValidationLayers))
    {
        throw std::runtime_error("validataion layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType                       = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName            = "VulkanApp";
    appInfo.applicationVersion          = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName                 = "No Engine";
    appInfo.engineVersion               = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion                  = VK_API_VERSION_1_0;

    auto extensions = VulkanUtils::getRequiredExtensions();

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo         = &appInfo;
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames  = extensions.data();

    if (gEnableValidationLayers)
    {
        createInfo.enabledLayerCount    = static_cast<uint32_t>(gValidationLayers.size());
        createInfo.ppEnabledLayerNames  = gValidationLayers.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        VulkanUtils::populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext                = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
    }
    else
    {
        createInfo.enabledLayerCount    = 0;
        createInfo.pNext                = nullptr;
    }


    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create instance!");
    }

    uint32_t vkExtensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, nullptr);
    std::vector<VkExtensionProperties> vkExtensions(vkExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &vkExtensionCount, vkExtensions.data());

    std::cout << "Available Extensions: " << vkExtensionCount << "\n";
    for (const auto& vkExtension : vkExtensions)
    {
        std::cout << "\t" << vkExtension.extensionName << "\n";
    }
}

void VulkanApp::setupDebugMessenger()
{
    if (!gEnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    VulkanUtils::populateDebugMessengerCreateInfo(createInfo);

    if (VulkanUtils::CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up debug messenger!");
    }

}

void VulkanApp::createSurface()
{
    if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create window surface!");
    }
}

void VulkanApp::pickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());
    for (const auto& device : devices)
    {
        if (VulkanUtils::isDeviceSuitable(device, surface_))
        {
            physicalDevice_ = device;
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

}

void VulkanApp::createLogicalDevice()
{
    QueueFamilyIndices indices = VulkanUtils::findQueueFamilies(physicalDevice_, surface_);

    const float queuePriority = 1.f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType               = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex    = queueFamily;
        queueCreateInfo.queueCount          = 1;
        queueCreateInfo.pQueuePriorities    = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos          = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount       = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures           = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount      = static_cast<uint32_t>(gDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames    = gDeviceExtensions.data();

    if (gEnableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount      = static_cast<uint32_t>(gValidationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames    = gValidationLayers.data();
    }
    else
    {
        deviceCreateInfo.enabledLayerCount      = 0;
    }

    if (vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Logical Device");
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
}

void VulkanApp::createSwapChain()
{
    const SwapChainSupportDetails swapChainSupport  = VulkanUtils::querySwapChainSupport(physicalDevice_, surface_);
    const VkSurfaceFormatKHR surfaceFormat          = VulkanUtils::chooseSwapSurfaceFormat(swapChainSupport.formats);
    const VkPresentModeKHR presentMode              = VulkanUtils::chooseSwapPresentMode(swapChainSupport.presentModes);
    const VkExtent2D extent                         = VulkanUtils::chooseSwapExtent(swapChainSupport.capabilities, window_);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType                        = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                      = surface_;
    createInfo.minImageCount                = imageCount;
    createInfo.imageFormat                  = surfaceFormat.format;
    createInfo.imageColorSpace              = surfaceFormat.colorSpace;
    createInfo.imageExtent                  = extent;
    createInfo.imageArrayLayers             = 1;
    createInfo.imageUsage                   = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = VulkanUtils::findQueueFamilies(physicalDevice_, surface_);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount    = 2;
        createInfo.pQueueFamilyIndices      = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform                 = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha               = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode                  = presentMode;
    createInfo.clipped                      = VK_TRUE;
    createInfo.oldSwapchain                 = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
    swapChainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

    swapChainImageFormat_ = surfaceFormat.format;
    swapChainExtent_ = extent;
}