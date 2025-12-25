
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "glfw.hpp"

namespace vke{

    GLFWInstance::GLFWInstance()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    GLFWInstance::~GLFWInstance()
    {
        glfwTerminate();
    }
    
    std::vector<const char*> GLFWInstance::getInstanceExtensions() const
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        return {glfwExtensions, glfwExtensions + glfwExtensionCount};
    }
    
    GLFWWindow::GLFWWindow(const GLFWInstance& glfwInstance, const vk::raii::Instance& instance, const CreateInfo& createInfo)
    {
        triggerSetFramebufferSize = createInfo.triggerSetFramebufferSize;

        window = glfwCreateWindow(createInfo.width, createInfo.height, createInfo.title, nullptr, nullptr);

        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
        {
            reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window))->triggerSetFramebufferSize(width, height);
        });

        {
            VkSurfaceKHR surface_;
            auto result = static_cast<vk::Result>(glfwCreateWindowSurface( *instance, window, nullptr, &surface_));
            vk::detail::resultCheck( result, "vke::GLFWWindow::GLFWWindow glfwCreateWindowSurface" );

            surface = vk::raii::SurfaceKHR{instance, surface_};
        }
    }

    GLFWWindow::~GLFWWindow()
    {
        glfwDestroyWindow(window);
    }

    vk::Extent2D GLFWWindow::getExtent2D() const
    {
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }

    bool GLFWWindow::shouldClose() const
    {
        return glfwWindowShouldClose(window);
    }
}