#define GLFW_INCLUDE_VULKAN

#include "render/backend/vulkan/vulkan_app.h"
#include <iostream>

#include "foundation/log/log_system.h"

LogSystem* gLoggerSystem = new LogSystem();

int main(int argc, char** argv)
{
    VulkanApp app;
    try
    {
        app.run();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
