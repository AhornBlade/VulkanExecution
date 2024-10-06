#pragma once

#include "utils.hpp"

namespace vke
{
	class Window
	{
	public:
		[[ nodiscard ]] explicit Window() = default;
		Window(const Window&) = default;
		Window& operator=(const Window&) = default;
		virtual ~Window() noexcept = default;

	protected:
		vk::raii::SurfaceKHR surface_{VK_NULL_HANDLE};
	};

}// namespace vke