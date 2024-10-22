#pragma once

#include "utils.hpp"

namespace vke{

	struct DeviceQueueRequirement
	{
		vk::QueueFlags queueType;
		std::vector<float> priorities = {0.5f};
	};

	class Queue
	{
	public:
		[[ nodiscard ]] Queue() = default;
		[[ nodiscard ]] Queue(
			const vk::raii::Device& device, 
			uint32_t queueFamilyIndex, 
			uint32_t queueIndex);

	private:
		vk::raii::Queue queue_{ VK_NULL_HANDLE };
	};

	struct QueueContextCreateInfo
	{
		uint32_t queueFamilyIndex;
		std::vector<uint32_t> queueIndices;
	};

	class QueueContext
	{
	public:
		[[ nodiscard ]] QueueContext() = default;
		[[ nodiscard ]] QueueContext(const vk::raii::Device& device, const QueueContextCreateInfo& createInfo);

	private:
		std::vector<Queue> queues_;
	};


} // namespace vke