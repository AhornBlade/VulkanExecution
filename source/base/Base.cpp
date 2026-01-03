#include "Base.hpp"

#include <iostream>

namespace vke{
    
	Instance::Instance(const CreateInfo& createInfo_)
	{
#ifdef _DEBUG
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
		debugCreateInfo.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugCreateInfo.setPfnUserCallback(
			[](vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				vk::DebugUtilsMessageTypeFlagsEXT messageType,
				const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData)
			{
				std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;

				return VK_FALSE;
			});
#endif
		{
			auto extensionProperties = context.enumerateInstanceExtensionProperties();
			auto layerProperties = context.enumerateInstanceLayerProperties();

			std::vector<const char*> extensions = std::ranges::to<std::vector<const char*>>(extensionProperties
				| std::ranges::views::filter(createInfo_.enabledExtensionChecker)
				| std::ranges::views::transform([](const vk::ExtensionProperties& p) -> const char*{ return p.extensionName; }));
			std::vector<const char*> layers =  std::ranges::to<std::vector<const char*>>(layerProperties
				| std::ranges::views::filter(createInfo_.enabledLayerChecker)
				| std::ranges::views::transform([](const vk::LayerProperties& p) -> const char*{ return p.layerName; }));
			
#ifdef _DEBUG
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			layers.emplace_back("VK_LAYER_KHRONOS_validation");
#endif

            // extensions.emplace_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
            // extensions.emplace_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

			vk::ApplicationInfo appInfo{};
			appInfo.setPApplicationName(createInfo_.applicationName);
			appInfo.setApplicationVersion(createInfo_.applicationVersion);
			appInfo.setPEngineName("VulkanExecution");
			appInfo.setEngineVersion(VK_MAKE_VERSION(0, 0, 1));
			appInfo.setApiVersion(vk::ApiVersion12);

			instance = vk::raii::Instance{ context, vk::InstanceCreateInfo{}
				.setPEnabledExtensionNames(extensions)
				.setPEnabledLayerNames(layers)
				.setPApplicationInfo(&appInfo) 
#ifdef _DEBUG
				.setPNext(&debugCreateInfo)
#endif
				};
		}

		debugMessenger = vk::raii::DebugUtilsMessengerEXT{instance, debugCreateInfo};
	}
    
    DeviceQueue::DeviceQueue(const vk::raii::Device& device_, uint32_t queueFamilyIndex_, uint32_t queueIndex_)
        : queueFamilyIndex(queueFamilyIndex_), queueIndex(queueIndex_)
        , queue{device_, queueFamilyIndex_, queueIndex_} {}

    Device::Device(const vk::raii::Instance& instance, const CreateInfo& createInfo_)
    {
        auto physicalDevices = instance.enumeratePhysicalDevices();

        Selecter<vk::raii::PhysicalDevice> physicalDeviceSelecter{createInfo_.physicalDevicSelecter.f_, 
            [&](const vk::raii::PhysicalDevice& p) -> bool
            {
                return !createInfo_.deviceQueueInfos(p).empty() && createInfo_.physicalDevicSelecter.checker(p);
            }};

        physicalDevice = physicalDeviceSelecter(physicalDevices);

        deviceQueueInfos = createInfo_.deviceQueueInfos(physicalDevice);

        std::vector<std::vector<float>> queuePriorities;
        queuePriorities.reserve(deviceQueueInfos.size());

        for(const auto& queueFamily : deviceQueueInfos)
        {
            std::vector<float> priorities{};
            priorities.reserve(queueFamily.size());

            for(const auto& queue : queueFamily)
            {
                priorities.emplace_back(queue.queuePriority);
            }

            queuePriorities.emplace_back(priorities);
        }

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};
        for(const auto& [index, queueFamily] : deviceQueueInfos | std::views::enumerate)
        {
            if(!queuePriorities[index].empty())
            {
                queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{}
                    .setQueueFamilyIndex(index)
                    .setQueuePriorities(queuePriorities[index]));
            }
        }

        auto extensionProperties = physicalDevice.enumerateDeviceExtensionProperties();

        auto enabledExtensions = std::ranges::to<std::vector<const char*>>(extensionProperties
            | std::ranges::views::filter(createInfo_.enabledExtensionChecker)
            | std::ranges::views::transform([](const vk::ExtensionProperties& p) -> const char*{ return p.extensionName; }));

        enabledExtensions.emplace_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

        auto enabledFeatures = createInfo_.enabledFeaturesTransformer(physicalDevice.getFeatures());

        device = std::make_unique<vk::raii::Device>(physicalDevice, vk::DeviceCreateInfo{}
            .setQueueCreateInfos(queueCreateInfos)
            .setPEnabledExtensionNames(enabledExtensions)
            .setPEnabledFeatures(&enabledFeatures));

        deviceQueues.resize(deviceQueueInfos.size());
        for(uint32_t queueFamilyIndex = 0; queueFamilyIndex < deviceQueueInfos.size(); queueFamilyIndex++)
        {
            auto& queueFamilies = deviceQueues[queueFamilyIndex];
            const auto& queueFamilyInfos = deviceQueueInfos[queueFamilyIndex];
            
            queueFamilies.reserve(queueFamilyInfos.size());
            for(uint32_t queueIndex = 0; queueIndex < queueFamilyInfos.size(); queueIndex++)
            {
                queueFamilies.emplace_back(DeviceQueue{*device, queueFamilyIndex, queueIndex});
            }
        }
    }

    const DeviceQueue& Device::getDeviceQueue(const std::function<uint32_t(const DeviceQueueInfo&)>& queueEvaluationFunction) const &
    {
        uint32_t bestQueueFamilyIndex = UINT32_MAX, bestQueueIndex = UINT32_MAX;
        uint32_t bestQueueScore = 0;

        for(const auto& [queueFamilyIndex, queueFamily] : deviceQueueInfos | std::views::enumerate)
        {
            for(const auto& [queueIndex, queue] : queueFamily | std::views::enumerate)
            {
                uint32_t queueScore = queueEvaluationFunction(queue);
                if( queueScore > bestQueueScore)
                {
                    bestQueueFamilyIndex = queueFamilyIndex;
                    bestQueueIndex = queueIndex;
                    bestQueueScore = queueScore;
                }
            }
        }

        if(bestQueueScore == 0)
        {
            throw std::runtime_error{std::format("Failed to find queue")};
        }

        return deviceQueues[bestQueueFamilyIndex][bestQueueIndex];
    }
}