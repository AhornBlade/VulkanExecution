#include "Queue.hpp"

#include <iostream>
#include <ranges>

namespace vke{

    Queue::Queue(const vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t queueIndex,
        vk::Semaphore contextSignalSemaphore)
        : queue_{device, queueFamilyIndex, queueFamilyIndex}, semaphore_{device},
        contextSignalSemaphore_{contextSignalSemaphore}
    {
        std::cout << "Success to create queue family: " << queueFamilyIndex << " index: " << queueIndex << '\n';
    }

    bool Queue::isIdle() const noexcept
    {
        return semaphore_.acquire(1) == time_status::no_timeout;
    }

    void Queue::submit(std::span<const vk::CommandBuffer> commandBuffers, 
        std::span<const vk::Semaphore> waitSemaphores,
        std::span<const vk::Semaphore> signalSemaphores_)
    {
        std::vector<vk::Semaphore> signalSemaphores{signalSemaphores_.begin(), signalSemaphores_.end()};

        signalSemaphores.push_back(semaphore_.get());
        signalSemaphores.push_back(contextSignalSemaphore_);

        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBuffers(commandBuffers);
        submitInfo.setWaitSemaphores(waitSemaphores);
        submitInfo.setSignalSemaphores(signalSemaphores);

        queue_.submit(submitInfo);
    }

    vk::Result Queue::presentKHR(std::span<vk::SwapchainKHR> swapchains, std::span<uint32_t> imageIndices, 
        std::span<const vk::Semaphore> waitSemaphores)
    {
        vk::PresentInfoKHR presentInfo{};

        presentInfo.setSwapchains(swapchains);
        presentInfo.setImageIndices(imageIndices);
        presentInfo.setWaitSemaphores(waitSemaphores);

        return queue_.presentKHR(presentInfo);
    }

    QueueContext::QueueContext(const vk::raii::Device& device, const QueueContextCreateInfo& createInfo)
    {
        for(uint32_t index : createInfo.queueIndices)
        {
            queues_.emplace_back(device, createInfo.queueFamilyIndex, index, cv_.get());
        }
    }

    void QueueContext::submit(QueueTask&& task) const
    {
        std::unique_lock lock{mutex_};

        auto check_func = [](const Queue& q){ return q.isIdle();};

        cv_.wait(lock, [&]{ return std::ranges::any_of(queues_, check_func); });
        
        task.submit_to(*std::ranges::find_if(queues_, check_func));
    }

} // namespace vke