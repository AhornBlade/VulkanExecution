#include "Memory.hpp"

#include <bit>

namespace vke{
    DeviceMemoryResource* default_device_memory_res = nullptr;

    DeviceMemoryInfo<void> DeviceMemoryResource::allocate(vk::MemoryRequirements requirements)
    {
        if(!std::has_single_bit(requirements.alignment))
        {
            throw std::runtime_error{"DeviceMemoryResource::allocate alignment must be a power of two"};
        }
        return do_allocate(requirements);
    }

    void DeviceMemoryResource::deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements)
    {
        if( !memory.memory )
            return;

        if(!std::has_single_bit(requirements.alignment))
        {
            throw std::runtime_error{"DeviceMemoryResource::allocate alignment must be a power of two"};
        }
        do_deallocate(memory, requirements);
    }

    bool DeviceMemoryResource::is_equal(const DeviceMemoryResource& other) const noexcept
    {
        return do_is_equal(other);
    }

    NewDeleteDeviceMemoryResource::NewDeleteDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter)
        : p_device{&device}, physicalDevice_{physicalDevice}
    {
        if(memoryTypeFlilter)
        {
            auto properties = physicalDevice.getMemoryProperties();

            memoryTypes = std::ranges::fold_left(std::ranges::views::iota(0u, properties.memoryTypeCount) 
                | std::ranges::views::filter([&](uint32_t index) -> bool
                    { 
                        auto& memoryType = properties.memoryTypes[index];
                        return memoryTypeFlilter(memoryType.propertyFlags, properties.memoryHeaps[memoryType.heapIndex]) ;
                    })
                | std::ranges::views::transform([](uint32_t index) -> uint32_t { return 1u << index; }), 0u, std::bit_or{});
        }
    }

    NewDeleteDeviceMemoryResource::NewDeleteDeviceMemoryResource(const Device& device, 
        std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter)
        : NewDeleteDeviceMemoryResource{device, device.getPhysicalDevice(), memoryTypeFlilter} {}
    
    DeviceMemoryInfo<void> NewDeleteDeviceMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t memoryTypeIndices = requirements.memoryTypeBits & memoryTypes;

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

        return DeviceMemoryInfo<void>{ new vk::raii::DeviceMemory{*p_device, vk::MemoryAllocateInfo{requirements.size, index}}, index };
    }

    void NewDeleteDeviceMemoryResource::do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements)
    {
        delete memory.memory;
    }

    bool NewDeleteDeviceMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const NewDeleteDeviceMemoryResource* p_other = dynamic_cast<const NewDeleteDeviceMemoryResource*>(&other);
        return p_other && (physicalDevice_ == p_other->physicalDevice_);
    }
    
    DeviceLocalMemoryResource::DeviceLocalMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream)
        : p_resource{upstream}
    {
        auto properties = physicalDevice.getMemoryProperties();

        for(uint32_t index = 0; index < properties.memoryTypeCount; index++)
        {
            if(properties.memoryTypes[index].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)
            {
                deviceLocalIndices |= (1u << index);
            }
        }
    }
    
    DeviceMemoryInfo<void> DeviceLocalMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t indices = requirements.memoryTypeBits & deviceLocalIndices;

        if(indices == 0)
        {
            throw std::runtime_error{"MappedDeviceMemoryResource::do_allocate, Failed to find valid memory type"};
        }

        return p_resource->allocate(vk::MemoryRequirements{requirements.size, requirements.alignment, indices});
    }

    void DeviceLocalMemoryResource::do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements)
    {
        p_resource->deallocate(memory, requirements);
    }

    bool DeviceLocalMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const DeviceLocalMemoryResource* p_other = dynamic_cast<const DeviceLocalMemoryResource*>(&other);
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
    
    DeviceMemoryInfo<void> MappedDeviceMemoryResource::do_allocate(vk::MemoryRequirements requirements)
    {
        uint32_t indices = requirements.memoryTypeBits & visibleIndices;

        if(indices == 0)
        {
            throw std::runtime_error{"MappedDeviceMemoryResource::do_allocate, Failed to find valid memory type"};
        }

        DeviceMemoryInfo<void> p = p_resource->allocate(vk::MemoryRequirements{requirements.size, requirements.alignment, indices});
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

    void MappedDeviceMemoryResource::do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements)
    {
        if( (coherentIndices & ( 1u << memory.memoryIndex )) == 0 )
        {
            vk::MappedMemoryRange range{*memory.memory, memory.offset, requirements.size};

            VULKAN_HPP_ASSERT( memory.memory->getDispatcher()->vkFlushMappedMemoryRanges && "Function <vkFlushMappedMemoryRanges> requires <VK_VERSION_1_0>" );

            vk::Result result = static_cast<vk::Result>( memory.memory->getDispatcher()->vkFlushMappedMemoryRanges( 
                static_cast<VkDevice>( memory.memory->getDevice() ), 1, reinterpret_cast<VkMappedMemoryRange*>(&range)) );
            vk::detail::resultCheck( result, "vke::MappedDeviceMemoryResource::do_allocate" );
        }

        p_resource->deallocate(memory, requirements);
    }

    bool MappedDeviceMemoryResource::do_is_equal(const DeviceMemoryResource& other) const noexcept
    {
        const MappedDeviceMemoryResource* p_other = dynamic_cast<const MappedDeviceMemoryResource*>(&other);
        return p_other && (p_resource == p_other->p_resource);
    }

    DeviceMemoryInfo<void> DeviceMemoryAllocator<void>::allocate_bytes(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        vk::MemoryRequirements requirements)
    {
        if(!p_resource)
        {
            p_resource = getDefaultDeviceMemoryResource(device, physicalDevice);
        }
        return p_resource->allocate(requirements);
    }

    void DeviceMemoryAllocator<void>::deallocate_bytes(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements)
    {
        if(!p_resource)
            return;
        p_resource->deallocate(memory, requirements);
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