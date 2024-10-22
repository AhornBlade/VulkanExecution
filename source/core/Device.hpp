#pragma once

#include <span>
#include <vector>
#include <functional>
#include "Feature.hpp"
#include "Queue.hpp"

namespace vke
{
	class Device
	{
	public:
		[[ nodiscard ]] explicit Device() = default;
		[[ nodiscard ]] Device(
			const vk::raii::Instance& instance,
			const Feature& feature, 
			std::span<const DeviceQueueRequirement> queueRequirement, 
			const std::function<uint32_t(const vk::PhysicalDeviceProperties&)>& scoreFunc = nullptr);

		[[ nodiscard ]] inline const vk::raii::Device& get() const noexcept { return device_; }
		inline const vk::raii::Device* operator->() const noexcept { return &device_; }

	private:
		vk::raii::PhysicalDevice physicalDevice_{ VK_NULL_HANDLE };
		vk::raii::Device device_{ VK_NULL_HANDLE };
		std::vector<QueueContext> queueContexts_{};
	};

}// namespace vke