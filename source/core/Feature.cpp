#include "Feature.hpp"

namespace vke{

    std::vector<const char*> Feature::getRequiredInstanceExtensionNames() const noexcept
    {
		std::vector<const char*> extensionNames{};

#ifdef VULKAN_EXECUTION_DEBUG
		extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // VULKAN_EXECUTION_DEBUG

		if (bUseWindow)
		{
			extensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VULKAN_EXECUTION_PLATFORM_SURFACE_EXTENSION_NAME
			extensionNames.push_back(VULKAN_EXECUTION_PLATFORM_SURFACE_EXTENSION_NAME);
#endif // VULKAN_EXECUTION_PLATFORM_SURFACE_EXTENSION_NAME
		}

		return extensionNames;
    }

    std::vector<const char*> Feature::getRequiredDeviceExtensionNames() const noexcept
    {
		std::vector<const char*> extensionNames{};

		if (bUseWindow)
		{
			extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		return extensionNames;
    }

    std::vector<const char*> Feature::getRequiredLayerNames() const noexcept
    {
		std::vector<const char*> layerNames{};

#ifdef VULKAN_EXECUTION_DEBUG
		layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif // VULKAN_EXECUTION_DEBUG

		return layerNames;
    }

    vk::PhysicalDeviceFeatures Feature::getRequiredDeviceFeature() const noexcept
    {
        return {};
    }

} // namespace vke