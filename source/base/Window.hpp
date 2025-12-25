#pragma once

#include "Common.hpp"

namespace vke{
    class WindowApplicationInstance
    {
    public:
        WindowApplicationInstance() = default;
        virtual ~WindowApplicationInstance() noexcept = default;
        
        WindowApplicationInstance(const WindowApplicationInstance&) = delete;
        WindowApplicationInstance& operator=(const WindowApplicationInstance&) = delete;

        WindowApplicationInstance(WindowApplicationInstance&&) = default;
        WindowApplicationInstance& operator=(WindowApplicationInstance&&) = default;

        virtual std::vector<const char*> getInstanceExtensions() const { return {}; }
        virtual std::vector<const char*> getDeviceExtensions() const { return { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; }
    };

    class Window
    {
    public:
        Window() = default;
        virtual ~Window() noexcept = default;

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        virtual vk::Extent2D getExtent2D() const = 0;
        virtual bool shouldClose() const = 0;

        inline const auto& getSurface() const & noexcept { return surface; }

    protected:
        vk::raii::SurfaceKHR surface{ nullptr };
        std::function<void(uint32_t width, uint32_t height)> triggerSetFramebufferSize;
    };

}