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
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device_);
}

void VulkanApp::cleanup()
{
    vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
    vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);

    vkDestroyCommandPool(device_, commandPool_, nullptr);

    for (auto framebuffer : swapChainFrameBuffers_)
    {
        vkDestroyFramebuffer(device_, framebuffer, nullptr);
    }

    vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    vkDestroyRenderPass(device_, renderPass_, nullptr);

    for (auto imageView : swapChainImageViews_)
    {
        vkDestroyImageView(device_, imageView, nullptr);
    }

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

void VulkanApp::createImageViews()
{
    swapChainImageViews_.resize(swapChainImages_.size());

    for (size_t index = 0; index < swapChainImageViews_.size(); index++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType                            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image                            = swapChainImages_[index];
        createInfo.viewType                         = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format                           = swapChainImageFormat_;
        createInfo.components.r                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a                     = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel    = 0;
        createInfo.subresourceRange.levelCount      = 1;
        createInfo.subresourceRange.baseArrayLayer  = 0;
        createInfo.subresourceRange.layerCount      = 1;

        if (vkCreateImageView(device_, &createInfo, nullptr, &swapChainImageViews_[index]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image views");
        }
    }
}

void VulkanApp::createRenderPass()
{
    VkAttachmentDescription colorAttchment{};
    colorAttchment.format           = swapChainImageFormat_;
    colorAttchment.samples          = VK_SAMPLE_COUNT_1_BIT;
    colorAttchment.loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttchment.storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttchment.stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttchment.stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttchment.initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttchment.finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment   = 0;
    colorAttachmentRef.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass           = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass           = 0;
    dependency.srcStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask        = 0;
    dependency.dstStageMask         = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask        = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = 1;
    renderPassInfo.pAttachments     = &colorAttchment;
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &dependency;

    if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create render pass");
    }
}

void VulkanApp::createGraphicsPipeline()
{
    auto vertShaderCode = VulkanUtils::readFile("E:/projects/learn_vulkan/shaders/vert.spv");
    auto fragShaderCode = VulkanUtils::readFile("E:/projects/learn_vulkan/shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType                       = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage                       = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module                      = vertShaderModule;
    vertShaderStageInfo.pName                       = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType                       = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage                       = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module                      = fragShaderModule;
    fragShaderStageInfo.pName                       = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                             = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology                          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable            = VK_FALSE;

    VkViewport viewport{};
    viewport.x                                      = 0.0f;
    viewport.y                                      = 0.0f;
    viewport.width                                  = static_cast<float> (swapChainExtent_.width);
    viewport.height                                 = static_cast<float> (swapChainExtent_.height);
    viewport.minDepth                               = 0.0f;
    viewport.maxDepth                               = 1.0f;

    VkRect2D scissor{};
    scissor.offset                                  = { 0, 0 };
    scissor.extent                                  = swapChainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount                     = 1;
    viewportState.pViewports                        = &viewport;
    viewportState.scissorCount                      = 1;
    viewportState.pScissors                         = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                                = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable                     = VK_FALSE;
    rasterizer.rasterizerDiscardEnable              = VK_FALSE;
    rasterizer.polygonMode                          = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth                            = 1.0f;
    rasterizer.cullMode                             = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace                            = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable                      = VK_FALSE;
    rasterizer.depthBiasConstantFactor              = 0.0f;
    rasterizer.depthBiasClamp                       = 0.0f;
    rasterizer.depthBiasSlopeFactor                 = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                             = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable               = VK_FALSE;
    multisampling.rasterizationSamples              = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading                  = 1.0f;
    multisampling.pSampleMask                       = nullptr;
    multisampling.alphaToCoverageEnable             = VK_FALSE;
    multisampling.alphaToOneEnable                  = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask             = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable                = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor        = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor        = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp               = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor        = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor        = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp               = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType                             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable                     = VK_FALSE;
    colorBlending.logicOp                           = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount                   = 1;
    colorBlending.pAttachments                      = &colorBlendAttachment;
    colorBlending.blendConstants[0]                 = 0.0f;
    colorBlending.blendConstants[1]                 = 0.0f;
    colorBlending.blendConstants[2]                 = 0.0f;
    colorBlending.blendConstants[3]                 = 0.0f;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType                              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount                  = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicState.pDynamicStates                     = dynamicStates;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount               = 0;
    pipelineLayoutInfo.pSetLayouts                  = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount       = 0;
    pipelineLayoutInfo.pPushConstantRanges          = nullptr;

    if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType                              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount                         = 2;
    pipelineInfo.pStages                            = shaderStages;
    pipelineInfo.pVertexInputState                  = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState                = &inputAssembly;
    pipelineInfo.pViewportState                     = &viewportState;
    pipelineInfo.pRasterizationState                = &rasterizer;
    pipelineInfo.pMultisampleState                  = &multisampling;
    pipelineInfo.pDepthStencilState                 = nullptr;
    pipelineInfo.pColorBlendState                   = &colorBlending;
    pipelineInfo.pDynamicState                      = nullptr;
    pipelineInfo.layout                             = pipelineLayout_;
    pipelineInfo.renderPass                         = renderPass_;
    pipelineInfo.subpass                            = 0;
    pipelineInfo.basePipelineHandle                 = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex                  = -1;

    if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device_, fragShaderModule, nullptr);
    vkDestroyShaderModule(device_, vertShaderModule, nullptr);
}

void VulkanApp::createFrameBuffers()
{
    swapChainFrameBuffers_.resize(swapChainImages_.size());

    for (size_t index = 0; index < swapChainImageViews_.size(); index++)
    {
        VkImageView attachments[] = { swapChainImageViews_[index] };

        VkFramebufferCreateInfo frameBufferInfo{};
        frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferInfo.renderPass = renderPass_;
        frameBufferInfo.attachmentCount = 1;
        frameBufferInfo.pAttachments = attachments;
        frameBufferInfo.width = swapChainExtent_.width;
        frameBufferInfo.height = swapChainExtent_.height;
        frameBufferInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &frameBufferInfo, nullptr, &swapChainFrameBuffers_[index]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void VulkanApp::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = VulkanUtils::findQueueFamilies(physicalDevice_, surface_);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool!");
    }
}

void VulkanApp::createCommandBuffers()
{
    commandBuffers_.resize(swapChainFrameBuffers_.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers!");
    }

    for (size_t index = 0; index < commandBuffers_.size(); index++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffers_[index], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass_;
        renderPassInfo.framebuffer = swapChainFrameBuffers_[index];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent_;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers_[index], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers_[index], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);
        vkCmdDraw(commandBuffers_[index], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers_[index]);

        if (vkEndCommandBuffer(commandBuffers_[index]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }
}

void VulkanApp::createSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create semaphores");
    }
}


VkShaderModule VulkanApp::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module");
    }

    return shaderModule;
}

void VulkanApp::drawFrame()
{
    uint32_t imageIndex;
    vkAcquireNextImageKHR(device_, swapChain_, UINT64_MAX, imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore_ };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphore_ };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &commandBuffers_[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkSwapchainKHR swapChains[] = { swapChain_ };

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount  = 1;
    presentInfo.pWaitSemaphores     = signalSemaphores;
    presentInfo.swapchainCount      = 1;
    presentInfo.pSwapchains         = swapChains;
    presentInfo.pImageIndices       = &imageIndex;
    presentInfo.pResults            = nullptr;

    vkQueuePresentKHR(presentQueue_, &presentInfo);

}