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
	#define VULKAN_EXECUTION_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__linux__)
	#define VK_USE_PLATFORM_XCB_KHR
	#define VULKAN_EXECUTION_PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XCB_SURFACE_EXTENSION_NAME
#else
	#error "Unsupported platform"
#endif

#include <vulkan/vulkan_raii.hpp>

# define VULKAN_EXECUTION_ENGINE_NAME "VulkanExecution"
# define VULKAN_EXECUTION_ENGINE_VERSION (100)