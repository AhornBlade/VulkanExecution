#pragma once

#include "utils.hpp"
#include "Synchronization.hpp"

#include <mutex>
#include <span>

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
		[[ nodiscard ]] Queue(const vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t queueIndex, 
			vk::Semaphore contextSignalSemaphore);

		[[ nodiscard ]] bool isIdle() const noexcept;
		void submit(std::span<const vk::CommandBuffer> commandBuffers, 
			std::span<const vk::Semaphore> waitSemaphores = {},
			std::span<const vk::Semaphore> signalSemaphores = {});
		[[ nodiscard ]] vk::Result presentKHR(std::span<vk::SwapchainKHR> swapchains, std::span<uint32_t> imageIndices, 
			std::span<const vk::Semaphore> waitSemaphores = {});

	private:
		vk::raii::Queue queue_{ VK_NULL_HANDLE };
		BinarySemaphore semaphore_;
		vk::Semaphore contextSignalSemaphore_;
	};

	struct QueueContextCreateInfo
	{
		uint32_t queueFamilyIndex;
		std::vector<uint32_t> queueIndices;
	};

	template<typename T>
	concept queue_submittable = requires(T t, const vk::raii::Queue& q ){ t.submit_to(q); };

	class QueueContext
	{
	public:
		[[ nodiscard ]] QueueContext() = default;
		[[ nodiscard ]] QueueContext(const vk::raii::Device& device, const QueueContextCreateInfo& createInfo);
		QueueContext(QueueContext&& other) noexcept : queues_(std::move(other.queues_)) {}
		QueueContext& operator=(QueueContext&& other) { std::swap(queues_, other.queues_); return *this; }

		void submit_to(queue_submittable auto&& task)
		{
			task.submit_to(selectIdleQueue());
		}

	private:
		std::vector<Queue> queues_;
		mutable std::mutex mutex_;
		mutable ConditionVariable cv_;

		[[ nodiscard ]] const Queue& selectIdleQueue() const noexcept;
	};


} // namespace vke