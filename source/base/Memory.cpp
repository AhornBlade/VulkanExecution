#include "Memory.hpp"

#include <ranges>

namespace vke{

    DefaultDeviceMemoryStrategy::MemoryState::MemoryState(const vk::raii::Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex)
        : memory_{device, vk::MemoryAllocateInfo{size, memoryTypeIndex}}, size_{size} {}
        
    DefaultDeviceMemoryStrategy::VisibleMemoryState::VisibleMemoryState(const vk::raii::Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex)
        : MemoryState{device, size, memoryTypeIndex}, data_{ memory_.mapMemory(0, size_) } {}

    DefaultDeviceMemoryStrategy::VisibleMemoryState::~VisibleMemoryState()
    {
        if(data_)
        {
            memory_.unmapMemory();
        }
    }

    auto DefaultDeviceMemoryStrategy::allocateMemory(const vk::MemoryRequirements& requirements) const -> MemoryState
    {
        uint32_t memoryTypeIndex = (std::ranges::views::iota(0u, 32u) 
            | std::ranges::views::filter([&](uint32_t index) { return (requirements.memoryTypeBits & (1 << index)); })).front();

        return MemoryState{ *device, requirements.size, memoryTypeIndex };
    }
    
    auto DefaultDeviceMemoryStrategy::allocateVisibleMemory(const vk::MemoryRequirements& requirements) const -> VisibleMemoryState
    {
        uint32_t memoryTypeIndex = (std::ranges::views::iota(0u, 32u) 
            | std::ranges::views::filter([&](uint32_t index) { return (requirements.memoryTypeBits & (1 << index)); })).front();

        return VisibleMemoryState{ *device, requirements.size, memoryTypeIndex };
    }
}