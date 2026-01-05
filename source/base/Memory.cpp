#include "Memory.hpp"

#include <bit>

namespace vke{
    DeviceMemoryResource* default_device_memory_res = nullptr;

    DeviceMemoryInfo DeviceMemoryResource::allocate(vk::MemoryRequirements requirements)
    {
        if(!std::has_single_bit(requirements.alignment))
        {
            throw std::runtime_error{"DeviceMemoryResource::allocate alignment must be a power of two"};
        }
        return do_allocate(requirements);
    }

    void DeviceMemoryResource::deallocate(DeviceMemoryInfo memory)
    {
        if( !memory.memory )
            return;

        do_deallocate(memory);
    }

    bool DeviceMemoryResource::is_equal(const DeviceMemoryResource& other) const noexcept
    {
        return do_is_equal(other);
    }

    NewDeleteDeviceMemoryResource::NewDeleteDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
        : p_device{&device}, physicalDevice_{physicalDevice} {}

    NewDeleteDeviceMemoryResource::NewDeleteDeviceMemoryResource(const Device& device)
        : NewDeleteDeviceMemoryResource{device, device.getPhysicalDevice()} {}
    
    DeviceMemoryInfo NewDeleteDeviceMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t memoryTypeIndices = requirements.memoryTypeBits;

        if(memoryTypeIndices == 0)
        {
            throw std::runtime_error{"NewDeleteDeviceMemoryResource::do_allocate, Failed to find valid memory type"};
        }
        
        auto [properties, budget] = physicalDevice_.getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();

        uint32_t index = (std::ranges::views::iota(0u, properties.memoryProperties.memoryTypeCount) 
            | std::ranges::views::filter([&](uint32_t index) -> bool
                { 
                    return (memoryTypeIndices & (1u << index)) && 
                        (budget.heapBudget[properties.memoryProperties.memoryTypes[index].heapIndex] >= requirements.size);
                })).front();

        return DeviceMemoryInfo{ new vk::raii::DeviceMemory{*p_device, vk::MemoryAllocateInfo{requirements.size, index}}, index, 0, requirements.size };
    }

    void NewDeleteDeviceMemoryResource::do_deallocate(DeviceMemoryInfo memory)
    {
        delete memory.memory;
    }

    bool NewDeleteDeviceMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const NewDeleteDeviceMemoryResource* p_other = dynamic_cast<const NewDeleteDeviceMemoryResource*>(&other);
        return p_other && (physicalDevice_ == p_other->physicalDevice_);
    }
    
    FilterDeviceMemoryResource::FilterDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream,
            std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter)
        : p_resource{upstream}
    {
        if(memoryTypeFlilter)
        {
            auto properties = physicalDevice.getMemoryProperties();

            validIndices = std::ranges::fold_left(std::ranges::views::iota(0u, properties.memoryTypeCount) 
                | std::ranges::views::filter([&](uint32_t index) -> bool
                    { 
                        auto& memoryType = properties.memoryTypes[index];
                        return memoryTypeFlilter(memoryType.propertyFlags, properties.memoryHeaps[memoryType.heapIndex]) ;
                    })
                | std::ranges::views::transform([](uint32_t index) -> uint32_t { return 1u << index; }), 0u, std::bit_or{});
        }
    }
    
    FilterDeviceMemoryResource::FilterDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, 
        DeviceMemoryResource* upstream, vk::MemoryPropertyFlags required)
        : FilterDeviceMemoryResource{ physicalDevice, upstream, 
            [required](vk::MemoryPropertyFlags property, vk::MemoryHeap heap) -> bool { return (property & required) == required; } } {}
    
    DeviceMemoryInfo FilterDeviceMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t indices = requirements.memoryTypeBits & validIndices;

        if(indices == 0)
        {
            throw std::runtime_error{"MappedDeviceMemoryResource::do_allocate, Failed to find valid memory type"};
        }

        return p_resource->allocate(vk::MemoryRequirements{requirements.size, requirements.alignment, indices});
    }

    void FilterDeviceMemoryResource::do_deallocate(DeviceMemoryInfo memory)
    {
        p_resource->deallocate(memory);
    }

    bool FilterDeviceMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const FilterDeviceMemoryResource* p_other = dynamic_cast<const FilterDeviceMemoryResource*>(&other);
        return p_other && (p_resource == p_other->p_resource);
    }
    
    MappedDeviceMemoryResource::MappedDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream)
        : p_resource{upstream}
    {
        auto properties = physicalDevice.getMemoryProperties();

        for(uint32_t index = 0; index < properties.memoryTypeCount; index++)
        {
            if(properties.memoryTypes[index].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)
            {
                visibleIndices |= (1u << index);
            }
            
            if(properties.memoryTypes[index].propertyFlags & vk::MemoryPropertyFlagBits::eHostCoherent)
            {
                coherentIndices |= (1u << index);
            }
        }
    }
    
    DeviceMemoryInfo MappedDeviceMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t indices = requirements.memoryTypeBits & visibleIndices;

        if(indices == 0)
        {
            throw std::runtime_error{"MappedDeviceMemoryResource::do_allocate, Failed to find valid memory type"};
        }

        DeviceMemoryInfo p = p_resource->allocate(vk::MemoryRequirements{requirements.size, requirements.alignment, indices});
        p.mapped = p.memory->mapMemory(0, requirements.size);

        if( (coherentIndices & ( 1u << p.memoryIndex )) == 0 )
        {
            vk::MappedMemoryRange range{*p.memory, p.offset, requirements.size};

            VULKAN_HPP_ASSERT( p.memory->getDispatcher()->vkInvalidateMappedMemoryRanges && "Function <vkInvalidateMappedMemoryRanges> requires <VK_VERSION_1_0>" );

            vk::Result result = static_cast<vk::Result>( p.memory->getDispatcher()->vkInvalidateMappedMemoryRanges( 
                static_cast<VkDevice>( p.memory->getDevice() ), 1, reinterpret_cast<VkMappedMemoryRange*>(&range)) );
            vk::detail::resultCheck( result, "vke::MappedDeviceMemoryResource::do_allocate" );
        }

        return p;
    }

    void MappedDeviceMemoryResource::do_deallocate(DeviceMemoryInfo memory)
    {
        if( (coherentIndices & ( 1u << memory.memoryIndex )) == 0 )
        {
            vk::MappedMemoryRange range{*memory.memory, memory.offset, memory.size};

            VULKAN_HPP_ASSERT( memory.memory->getDispatcher()->vkFlushMappedMemoryRanges && "Function <vkFlushMappedMemoryRanges> requires <VK_VERSION_1_0>" );

            vk::Result result = static_cast<vk::Result>( memory.memory->getDispatcher()->vkFlushMappedMemoryRanges( 
                static_cast<VkDevice>( memory.memory->getDevice() ), 1, reinterpret_cast<VkMappedMemoryRange*>(&range)) );
            vk::detail::resultCheck( result, "vke::MappedDeviceMemoryResource::do_allocate" );
        }

        p_resource->deallocate(memory);
    }

    bool MappedDeviceMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const MappedDeviceMemoryResource* p_other = dynamic_cast<const MappedDeviceMemoryResource*>(&other);
        return p_other && (p_resource == p_other->p_resource);
    }

    
    QueueTransferMemoryResource::QueueTransferMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        DeviceMemoryResource* deviceLocalUpstream, DeviceMemoryResource* stagingUpstream)
        : p_device{&device},
            deviceLocal(physicalDevice, deviceLocalUpstream, vk::MemoryPropertyFlagBits::eDeviceLocal), 
            coherent(physicalDevice, stagingUpstream, vk::MemoryPropertyFlagBits::eHostCoherent),
            staging(physicalDevice, &coherent) {}
    
    QueueTransferMemoryResource::QueueTransferMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        DeviceMemoryResource* upstream)
        : QueueTransferMemoryResource{device, physicalDevice, upstream, upstream} {}

    QueueTransferMemoryResource::QueueTransferMemoryResource(const Device& device, DeviceMemoryResource* deviceLocalUpstream, DeviceMemoryResource* stagingUpstream)
        : QueueTransferMemoryResource{device, device.getPhysicalDevice(), deviceLocalUpstream, stagingUpstream } {}

    QueueTransferMemoryResource::QueueTransferMemoryResource(const Device& device, DeviceMemoryResource* upstream) 
        : QueueTransferMemoryResource{ device, device.getPhysicalDevice(), upstream } {}
        
    void QueueTransferMemoryResource::cmdUpload(const vk::raii::CommandBuffer& commandBuffer) const
    {
        for(auto& [_, info] : map)
        {
            commandBuffer.copyBuffer(info.staging, info.deviceLocal, {0, 0, info.memory.size});
        }
    }
    
    DeviceMemoryInfo QueueTransferMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        DeviceMemoryInfo deviceLocalMemory = deviceLocal.allocate(requirements);
        DeviceMemoryInfo stagingMemory = staging.allocate(requirements);

        vk::BufferCreateInfo createInfo{{}, requirements.size, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst};

        deviceLocalMemory.mapped = stagingMemory.mapped;
        map[deviceLocalMemory.mapped] = {stagingMemory, vk::raii::Buffer{*p_device, createInfo}, vk::raii::Buffer{*p_device, createInfo} };

        return deviceLocalMemory;
    }

    void QueueTransferMemoryResource::do_deallocate(DeviceMemoryInfo memory)
    {
        if(auto it = map.find(memory.mapped); it != map.end() )
        {
            staging.deallocate(it->second.memory);
            map.erase(it);
        }
        memory.mapped = nullptr;
        deviceLocal.deallocate(memory);
    }

    bool QueueTransferMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        return this == &other;
    }
    
    DeviceMemoryResource* getDefaultDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
    {
        if(!default_device_memory_res)
        {
            default_device_memory_res = new MappedDeviceMemoryResource{physicalDevice, new NewDeleteDeviceMemoryResource{device, physicalDevice} };
        }

        return default_device_memory_res;
    }

    DeviceMemoryResource* setDefaultDeviceMemoryResource(DeviceMemoryResource* resource)
    {
        default_device_memory_res = resource;
        return default_device_memory_res;
    }
}