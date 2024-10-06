#pragma once

#include "utils.hpp"

namespace vke
{
	class Window
	{
	public:
		[[ nodiscard ]] explicit Window() = default;
		Window(Window&&) noexcept = default;
		Window& operator=(Window&&) noexcept = default;
		virtual ~Window() noexcept = default;

	protected:
		vk::raii::SurfaceKHR surface_{VK_NULL_HANDLE};
	};

}// namespace vke