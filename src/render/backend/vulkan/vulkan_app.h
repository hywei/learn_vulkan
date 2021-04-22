#pragma once

#include "render/backend/vulkan/vulkan_config.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <vector>

class VulkanApp {
public:
    virtual ~VulkanApp() = default;

    virtual void run();

protected:
    void initWindow();
    void initVulkan();
    void mainLoop();

    // release resources
    void cleanupSwapChain();
    void cleanup();

    // create resources
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFrameBuffers();
    void createCommandPool();
    void createDepthResources();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    void recreateSwapChain();

    // helper functions
    VkShaderModule  createShaderModule(const std::vector<char>& code) const;
    void            createBuffer(VkDeviceSize          size,
                                 VkBufferUsageFlags    usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer&             buffer,
                                 VkDeviceMemory&       bufferMemory) const;
    void            copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void            copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
    void            createImage(uint32_t              width,
                                uint32_t              height,
                                VkFormat              format,
                                VkImageTiling         tiling,
                                VkImageUsageFlags     usage,
                                VkMemoryPropertyFlags properties,
                                VkImage&              image,
                                VkDeviceMemory&       imageMemory) const;
    VkImageView     createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;
    uint32_t        findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat        findDepthFormat() const;
    void            updateUniformBuffer(uint32_t imageIndex);
    VkCommandBuffer beginSingleTimeCommands() const;
    void            endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;

    void drawFrame();

    static void frameBufferResizeCallback(GLFWwindow* windows, int width, int height);

private:
    GLFWwindow*                  window_ {nullptr};
    VkInstance                   instance_ {};
    VkDebugUtilsMessengerEXT     debugMessenger_ {};
    VkPhysicalDevice             physicalDevice_ {nullptr};
    VkDevice                     device_ {nullptr};
    VkQueue                      graphicsQueue_ {};
    VkQueue                      presentQueue_ {};
    VkSurfaceKHR                 surface_ {};
    VkSwapchainKHR               swapChain_ {};
    VkFormat                     swapChainImageFormat_ {};
    VkExtent2D                   swapChainExtent_ {};
    std::vector<VkImage>         swapChainImages_;
    std::vector<VkImageView>     swapChainImageViews_;
    std::vector<VkFramebuffer>   swapChainFrameBuffers_;
    VkRenderPass                 renderPass_ {};
    VkDescriptorSetLayout        descriptorSetLayout_ {};
    VkPipelineLayout             pipelineLayout_ {};
    VkPipeline                   graphicsPipeline_ {};
    VkCommandPool                commandPool_ {};
    VkDescriptorPool             descriptorPool_ {};
    VkImage                      depthImage_ {};
    VkDeviceMemory               depthImageMemory_ {};
    VkImageView                  depthImageView_ {};
    VkImage                      textureImage_ {};
    VkDeviceMemory               textureImageMemory_ {};
    VkImageView                  textureImageView_ {};
    VkSampler                    textureSampler_ {};
    VkBuffer                     vertexBuffer_ {};
    VkDeviceMemory               vertexBufferMemory_ {};
    VkBuffer                     indexBuffer_ {};
    VkDeviceMemory               indexBufferMemory_ {};
    std::vector<VkBuffer>        uniformBuffers_;
    std::vector<VkDeviceMemory>  uniformBuffersMemory_;
    std::vector<VkDescriptorSet> descriptorSets_;
    std::vector<VkCommandBuffer> commandBuffers_;
    std::vector<VkSemaphore>     imageAvailableSemaphores_ {};
    std::vector<VkSemaphore>     renderFinishedSemaphores_ {};
    std::vector<VkFence>         inFlightFences_ {};
    std::vector<VkFence>         imagesInFlight_ {};
    size_t                       currentFrameIndex_ {0};
    bool                         frameBufferResized_ {false};
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
