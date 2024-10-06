#pragma once

#if defined(_WIN32)
	#include <windows.h>
	#ifdef min
		#undef min
	#endif
	#ifdef max
		#undef max
	#endif
	#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
	#define VK_USE_PLATFORM_XCB_KHR
#else
	#error "Unsupported platform"
#endif

#include <vulkan/vulkan_raii.hpp>

namespace vke
{
# define VULKAN_EXECUTION_ENGINE_NAME "VulkanExecution"
# define VULKAN_EXECUTION_ENGINE_VERSION (100)

	struct Features
	{
	};

}// namespace vke