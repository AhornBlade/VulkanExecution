#include "Device.hpp"

#include <stdexcept>
#include <ranges>
#include <vector>
#include <span>
#include <iostream>
#include <numeric>

namespace vke
{
	bool checkPhysicalDeviceExtension(
		const vk::raii::PhysicalDevice& physicalDevice,
		std::span<const char* const> extensionNames)
	{
		return std::ranges::includes(physicalDevice.enumerateDeviceExtensionProperties()
			| std::views::transform([](const vk::ExtensionProperties& pro) -> const char*
		{
			return pro.extensionName.data();
		}), extensionNames, std::strcmp);
	}

	bool checkPhysicalDeviceFeatures(
		const vk::raii::PhysicalDevice& physicalDevice, 
		const vk::PhysicalDeviceFeatures requiredFeatures)
	{	
		return physicalDevice.getFeatures() >= requiredFeatures;
	}

	struct DeviceQueueCreateInfo
	{
		std::vector<QueueContextCreateInfo> contextCreateInfos;
		std::vector<float> queuePriorities;
	};

	std::vector<vk::DeviceQueueCreateInfo> transferDeviceQueueCreateInfo(const DeviceQueueCreateInfo& queueCreateInfo)
	{
		std::vector<uint32_t> queueCounts;

		for(const auto& contextCreateInfo : queueCreateInfo.contextCreateInfos)
		{
			if(contextCreateInfo.queueFamilyIndex >= queueCounts.size())
			{
				queueCounts.resize(contextCreateInfo.queueFamilyIndex + 1);
			}

			queueCounts[contextCreateInfo.queueFamilyIndex] += contextCreateInfo.queueIndices.size();
		}

		std::vector<vk::DeviceQueueCreateInfo> createInfos;

		uint32_t PropertyIndex = 0;

		for(auto&& [queueFamilyIndex, queueCount] : std::views::enumerate(queueCounts) )
		{
			vk::DeviceQueueCreateInfo createInfo{};

			createInfo.setQueueFamilyIndex(queueFamilyIndex);
			createInfo.setPQueuePriorities(queueCreateInfo.queuePriorities.data() + PropertyIndex);
			createInfo.setQueueCount(queueCount);

			createInfos.push_back(createInfo);

			PropertyIndex += queueCount;
		}

		return createInfos;
	}

	DeviceQueueCreateInfo getPhysicalDeviceQueueCreateInfo(
		const vk::raii::PhysicalDevice& physicalDevice, 
		std::span<const DeviceQueueRequirement> queueRequirement)
	{
		auto queueFamilies = physicalDevice.getQueueFamilyProperties();

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

		DeviceQueueCreateInfo createInfo{};

		auto func = [](uint32_t queueFamilyIndex, uint32_t queueCount, uint32_t beginQueueIndex)
		{
			QueueContextCreateInfo contextCreateInfo{};
			contextCreateInfo.queueFamilyIndex = queueFamilyIndex;
			contextCreateInfo.queueIndices.resize(queueCount);
			std::ranges::iota(contextCreateInfo.queueIndices, beginQueueIndex);

			return contextCreateInfo;
		};

		std::vector<VkBool32> requirementTag(queueRequirement.size(), vk::True);
		
		for(auto&& [queueFamilyIndex, queueFamily] : std::views::enumerate(queueFamilies) )
		{
			uint32_t queueIndex = 0;

			for(uint32_t index = 0; index < queueRequirement.size(); index++)
			{
				const auto& requirement = queueRequirement[index];
				uint32_t queueCount = requirement.priorities.size();

				if((queueFamily.queueFlags & requirement.queueType) &&
					(queueFamily.queueCount > queueIndex + queueCount) &&
					requirementTag[index])
				{
					createInfo.contextCreateInfos.emplace_back(func(queueFamilyIndex, queueCount, queueIndex));
					createInfo.queuePriorities.append_range(requirement.priorities);
					queueIndex += queueCount;
					requirementTag[index] = vk::False;
				}
			}
		}

		if(createInfo.contextCreateInfos.size() < queueRequirement.size())
		{
			return {};
		}

		return createInfo;
	}

	Device::Device(
		const vk::raii::Instance& instance,
		const Feature& feature, 
		std::span<const DeviceQueueRequirement> queueRequirement, 
		const std::function<uint32_t(const vk::PhysicalDeviceProperties&)>& scoreFunc)
	{
		std::vector<const char*> extensionNames = feature.getRequiredDeviceExtensionNames();
		vk::PhysicalDeviceFeatures requiredFeatures = feature.getRequiredDeviceFeature();
		DeviceQueueCreateInfo queueCreateInfos;
		
		uint32_t hightestScore = 0;

		for (const auto& physicalDevice : instance.enumeratePhysicalDevices())
		{
			if(!checkPhysicalDeviceExtension(physicalDevice, extensionNames) || 
				!checkPhysicalDeviceFeatures(physicalDevice, requiredFeatures))
				{
					continue;
				}

			queueCreateInfos = getPhysicalDeviceQueueCreateInfo(physicalDevice, queueRequirement);

			if(queueCreateInfos.contextCreateInfos.empty())
			{
				continue;
			}

			if(!scoreFunc)
			{
				physicalDevice_ = physicalDevice;
				hightestScore = 1;
				break;
			}

			uint32_t score = scoreFunc(physicalDevice.getProperties());

			if (score > hightestScore)
			{
				physicalDevice_ = physicalDevice;
				hightestScore = score;
			}
		}

		if (hightestScore == 0)
		{
			throw std::runtime_error{ "Failed to find suitable physical devices" };
		}

		std::cout << "Success to find physical device: " << physicalDevice_.getProperties().deviceName << '\n';

		std::vector<const char*> layerNames = feature.getRequiredLayerNames();
		auto deviceQueueCreateInfo = transferDeviceQueueCreateInfo(queueCreateInfos);

		vk::DeviceCreateInfo createInfo{};
		createInfo.setPEnabledExtensionNames(extensionNames);
		createInfo.setPEnabledLayerNames(layerNames);
		createInfo.setPEnabledFeatures(&requiredFeatures);
		createInfo.setQueueCreateInfos(deviceQueueCreateInfo);

		device_ = vk::raii::Device{ physicalDevice_, createInfo };

		for(const auto& contextCreateInfo : queueCreateInfos.contextCreateInfos)
		{
			std::cout << "Begin to create queue context\n";
			queueContexts_.emplace_back(device_, contextCreateInfo);
		}
	}
}