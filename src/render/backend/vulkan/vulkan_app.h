#pragma once
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
    void createGraphicsPipeline();
    void createFrameBuffers();
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
    void createCommandBuffers();
    void createSyncObjects();

    void recreateSwapChain();

    // helper functions
    VkShaderModule createShaderModule(const std::vector<char>& code) const;
    void           createBuffer(VkDeviceSize          size,
                                VkBufferUsageFlags    usage,
                                VkMemoryPropertyFlags properties,
                                VkBuffer&             buffer,
                                VkDeviceMemory&       bufferMemory) const;
    void           copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t       findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

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
    VkPipelineLayout             pipelineLayout_ {};
    VkPipeline                   graphicsPipeline_ {};
    VkCommandPool                commandPool_ {};
    VkBuffer                     vertexBuffer_ {};
    VkDeviceMemory               vertexBufferMemory_ {};
    VkBuffer                     indexBuffer_ {};
    VkDeviceMemory               indexBufferMemory_ {};
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
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription                  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
};