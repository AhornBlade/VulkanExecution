#include "Memory.hpp"

#include <ranges>

namespace vke{

    DeviceMemoryBase::DeviceMemoryBase(DeviceMemoryInfo memoryInfo_)
        :memoryInfo{ memoryInfo_ } {}

    DeviceMemoryBase::~DeviceMemoryBase()
    {
        if(memoryInfo.allocator)
        {
            memoryInfo.allocator->freeMemory(*this);
        }
    }
    
    void* DeviceMemoryBase::mapMemory()
    {
        if(memoryInfo.block)
        {
            return memoryInfo.block->mapMemory(memoryInfo.offset, memoryInfo.size);
        }
        return nullptr;
    }

    void DeviceMemoryBase::bind(const vk::raii::Buffer& buffer) const
    {
        buffer.bindMemory(*memoryInfo.block, memoryInfo.offset);
    }

    void DeviceMemoryBase::bind(const vk::raii::Image& image) const
    {
        image.bindMemory(*memoryInfo.block, memoryInfo.offset);
    }

    DeviceMemoryBlockBase::DeviceMemoryBlockBase(vk::raii::DeviceMemory&& memory_, DeviceMemoryAllocatorBase& allocator_)
        : memory{ std::move(memory_) } {}

    DeviceMemoryBlockBase::~DeviceMemoryBlockBase()
    {
        if(data)
        {
            memory.unmapMemory();
        }
    }

    void* DeviceMemoryBlockBase::  mapMemory( vk::DeviceSize offset, vk::DeviceSize size, vk::MemoryMapFlags flags)
    {
        if(!data)
        {
            data = memory.mapMemory(0, vk::WholeSize, flags);
        }

        return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data) + offset);
    }

    DefaultDeviceMemoryAllocator::DefaultDeviceMemoryAllocator(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        const std::function<bool(vk::MemoryPropertyFlags, vk::MemoryHeap)>& filterFunction)
        : pDevice{&device}
    {
        if(filterFunction)
        {
            auto properties = physicalDevice.getMemoryProperties();
            validMemoryIndex = 0;

            for(uint32_t index = 0; index < properties.memoryTypeCount; index++)
            {
                const auto& memoryType = properties.memoryTypes[index];

                if(filterFunction(memoryType.propertyFlags, properties.memoryHeaps[memoryType.heapIndex]))
                {
                    validMemoryIndex |= (1 << index);
                }
            }
        }
    }

    DefaultDeviceMemoryAllocator::DefaultDeviceMemoryAllocator(const Device& device, 
        const std::function<bool(vk::MemoryPropertyFlags, vk::MemoryHeap)>& filterFunction)
        : DefaultDeviceMemoryAllocator{device, device.getPhysicalDevice(), filterFunction} {}
    
    std::unique_ptr<DeviceMemoryBase> DefaultDeviceMemoryAllocator::allocateMemory(const vk::MemoryRequirements& requirements)
    {
        if (!(requirements.memoryTypeBits & validMemoryIndex))
        {
            throw std::runtime_error("Failed to allocate memory, invalid required memory type");
        }

        uint32_t memoryTypeIndex = (std::ranges::views::iota(0u, 32u) 
            | std::ranges::views::filter([&](uint32_t index) { return (requirements.memoryTypeBits & validMemoryIndex & (1 << index)); })).front();

        return std::make_unique<DeviceMemoryBase>(DeviceMemoryInfo{this, new DeviceMemoryBlockBase(vk::raii::DeviceMemory{*pDevice,
            vk::MemoryAllocateInfo{requirements.size, memoryTypeIndex}}, *this), 0, requirements.size});
    }

    void DefaultDeviceMemoryAllocator::freeMemory(DeviceMemoryBase& memory)
    {
        delete memory.memoryInfo.block;
    }
    
    MappableAllocator<DeviceMemoryAllocatorBase>::MappableAllocator
        (const vk::raii::PhysicalDevice& physicalDevice, std::unique_ptr<DeviceMemoryAllocatorBase> && allocator_)
        : allocator{ std::move(allocator_) }
    {
        auto properties = physicalDevice.getMemoryProperties();

        for(uint32_t index = 0; index < properties.memoryTypeCount; index++)
        {
            if(properties.memoryTypes[index].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                mappableMemoryIndex |= (1 << index);
            }
        }
    }
    
    std::unique_ptr<DeviceMemoryBase> MappableAllocator<DeviceMemoryAllocatorBase>::allocateMemory(const vk::MemoryRequirements& requirements)
    {
        return allocator->allocateMemory(vk::MemoryRequirements{requirements.size, requirements.alignment, (requirements.memoryTypeBits & mappableMemoryIndex)});
    }

    void MappableAllocator<DeviceMemoryAllocatorBase>::freeMemory(DeviceMemoryBase& memory)
    {
        allocator->freeMemory(memory);
    }
}