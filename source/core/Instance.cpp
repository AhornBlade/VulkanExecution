#include "Instance.hpp"

#include <iostream>
#include <vector>

namespace vke
{
	Instance::Instance(const Features& feature, std::string_view applicationName, uint32_t applicationVersion)
	{
#ifdef VULKAN_EXECUTION_DEBUG
		vk::DebugUtilsMessengerCreateInfoEXT debugInfo{};
		debugInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
		debugInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugInfo.setPfnUserCallback([](
			VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData)->VkBool32
			{
				if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
				{
					std::cout << "\033[32m""Info: " << pCallbackData->pMessage << "\n""\033[0m";
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				{
					std::cout << "\033[33m""Warning: " << pCallbackData->pMessage << "\n""\033[0m";
				}
				else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				{
					std::cout << "\033[31m""Error: " << pCallbackData->pMessage << "\n""\033[0m";
				}

				return VK_FALSE;
			});
#endif // VULKAN_EXECUTION_DEBUG

		{
			std::vector<const char*> extensionNames;
			std::vector<const char*> layerNames;
#ifdef VULKAN_EXECUTION_DEBUG
			extensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			layerNames.push_back("VK_LAYER_KHRONOS_validation");
#endif // VULKAN_EXECUTION_DEBUG

			vk::ApplicationInfo appInfo{};
			appInfo.setApiVersion(context_.enumerateInstanceVersion());
			appInfo.setEngineVersion(VULKAN_EXECUTION_ENGINE_VERSION);
			appInfo.setPEngineName(VULKAN_EXECUTION_ENGINE_NAME);
			appInfo.setApplicationVersion(applicationVersion);
			appInfo.setPApplicationName(applicationName.data());

			vk::InstanceCreateInfo instanceCreateInfo{};
			instanceCreateInfo.setPApplicationInfo(&appInfo);
			instanceCreateInfo.setPEnabledExtensionNames(extensionNames);
			instanceCreateInfo.setPEnabledLayerNames(layerNames);
#ifdef VULKAN_EXECUTION_DEBUG
			instanceCreateInfo.setPNext(&debugInfo);
#endif // VULKAN_EXECUTION_DEBUG

			instance_ = vk::raii::Instance{ context_, instanceCreateInfo };
		}

#ifdef VULKAN_EXECUTION_DEBUG
		debugMessage_ = vk::raii::DebugUtilsMessengerEXT{ instance_, debugInfo };
#endif // VULKAN_EXECUTION_DEBUG
	}

}// namespace vke