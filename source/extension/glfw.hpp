#pragma once

#define NOMINMAX

#include <base/Window.hpp>

namespace vke{

    class GLFWInstance final : public WindowApplicationInstance
    {
    public:
        explicit GLFWInstance();
        ~GLFWInstance();

        std::vector<const char*> getInstanceExtensions() const override;
    };

    class GLFWWindow final : public Window
    {
    public:
        struct CreateInfo
        {
            uint32_t width;
            uint32_t height;
            const char* title = nullptr;
            std::function<void(uint32_t width, uint32_t height)> triggerSetFramebufferSize;
        };

        GLFWWindow(const GLFWInstance& glfwInstance, const vk::raii::Instance& instance, const CreateInfo& createInfo);
        ~GLFWWindow();

        vk::Extent2D getExtent2D() const override;
        bool shouldClose() const override;

    private:
        class GLFWwindow* window = nullptr;
    };

}