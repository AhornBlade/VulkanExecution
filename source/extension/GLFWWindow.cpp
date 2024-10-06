#include "GLFWWindow.hpp"

#include <GLFW/glfw3.h>

namespace vke
{
	GLFWWindow::GLFWWindow(const vk::raii::Instance& instance, int width, int height, std::string_view title)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
		glfwSetWindowUserPointer(window_, this);
		glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* window, int width, int height)
			{
				auto app = reinterpret_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
				app->framebufferResized = true;
		});

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		glfwCreateWindowSurface(*instance, window_, nullptr, &surface);

		surface_ = vk::raii::SurfaceKHR{ instance, surface };
	}

	GLFWWindow::~GLFWWindow()
	{
		glfwDestroyWindow(window_);
	}

}// namespace vke