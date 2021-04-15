#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

class VulkanApp
{
public:
    virtual ~VulkanApp() = default;

    virtual void run();

protected:
    void initWindow();
    void initVulkan();
    void mainLoop();

    // release resources
    void cleanupSwapChain();
    void cleanup();;

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
    void createCommandBuffers();
    void createSyncObjects();

    void recreateSwapChain();

    VkShaderModule createShaderModule(const std::vector<char>& code) const;

    void drawFrame();

    static void frameBufferResizeCallback(GLFWwindow* windows, int width, int height);
private:
    GLFWwindow*                     window_{ nullptr };
    VkInstance                      instance_{};
    VkDebugUtilsMessengerEXT        debugMessenger_{};
    VkPhysicalDevice                physicalDevice_{ VK_NULL_HANDLE };
    VkDevice                        device_{ VK_NULL_HANDLE };
    VkQueue                         graphicsQueue_{};
    VkQueue                         presentQueue_{};
    VkSurfaceKHR                    surface_{};
    VkSwapchainKHR                  swapChain_{};
    VkFormat                        swapChainImageFormat_{};
    VkExtent2D                      swapChainExtent_{};
    std::vector<VkImage>            swapChainImages_;
    std::vector<VkImageView>        swapChainImageViews_;
    std::vector<VkFramebuffer>      swapChainFrameBuffers_;
    VkRenderPass                    renderPass_{};
    VkPipelineLayout                pipelineLayout_{};
    VkPipeline                      graphicsPipeline_{};
    VkCommandPool                   commandPool_{};
    std::vector<VkCommandBuffer>    commandBuffers_;
    std::vector<VkSemaphore>        imageAvailableSemaphores_{};
    std::vector<VkSemaphore>        renderFinishedSemaphores_{};
    std::vector<VkFence>            inFlightFences_{};
    std::vector<VkFence>            imagesInFlight_{};
    size_t                          currentFrameIndex_{ 0 };
    bool                            frameBufferResized_{ false };
};