#include "Queue.hpp"

#include <iostream>

namespace vke{

    Queue::Queue(
        const vk::raii::Device& device, 
        uint32_t queueFamilyIndex, uint32_t 
        queueIndex)
    {
        std::cout << "Success to create queue family: " << queueFamilyIndex << " index: " << queueIndex << '\n';
        queue_ = device.getQueue(queueFamilyIndex, queueIndex);
    }

    QueueContext::QueueContext(const vk::raii::Device& device, const QueueContextCreateInfo& createInfo)
    {
        for(uint32_t index : createInfo.queueIndices)
        {
            queues_.emplace_back(device, createInfo.queueFamilyIndex, index);
        }
    }

} // namespace vke