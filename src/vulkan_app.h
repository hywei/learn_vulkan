#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <vector>

class VulkanApp
{
public:
    virtual void run();


protected:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();;

    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();

    //VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    //VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;


private:
    GLFWwindow*                 window_{ nullptr };
    VkInstance                  instance_{};
    VkDebugUtilsMessengerEXT    debugMessenger_{};
    VkPhysicalDevice            physicalDevice_{ VK_NULL_HANDLE };
    VkDevice                    device_{ VK_NULL_HANDLE };
    VkQueue                     graphicsQueue_{};
    VkQueue                     presentQueue_{};
    VkSurfaceKHR                surface_{};
    VkSwapchainKHR              swapChain_{};
    std::vector<VkImage>        swapChainImages_{};
    VkFormat                    swapChainImageFormat_{};
    VkExtent2D                  swapChainExtent_{};
};