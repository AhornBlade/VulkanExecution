#pragma once

#include "../core/Window.hpp"

class GLFWwindow;

namespace vke
{
	class GLFWWindow final : public Window
	{
	public:
		[[ nodiscard ]] GLFWWindow() = default;
		[[ nodiscard ]] GLFWWindow(const vk::raii::Instance& instance, int width, int height, std::string_view title);
		~GLFWWindow() override;

	private:
		GLFWwindow* window_ = nullptr;
		bool framebufferResized;
	};

}// namespace vke