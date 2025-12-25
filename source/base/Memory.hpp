#pragma once

#include "Base.hpp"

namespace vke{

    class DeviceMemoryAllocatorBase;
    class DeviceMemoryBlockBase;

    struct DeviceMemoryInfo
    {
        DeviceMemoryAllocatorBase* allocator = nullptr;
        DeviceMemoryBlockBase* block = nullptr;
        vk::DeviceSize offset = 0;
        vk::DeviceSize size = 0;
    };

    class DeviceMemoryBase
    {
    public:
        DeviceMemoryBase(DeviceMemoryInfo memoryInfo);
        virtual ~DeviceMemoryBase();

        DeviceMemoryBase(const DeviceMemoryBase&) = delete;
        DeviceMemoryBase& operator=(const DeviceMemoryBase&) = delete;
        DeviceMemoryBase(DeviceMemoryBase&&) = default;
        DeviceMemoryBase& operator=(DeviceMemoryBase&&) = default;

        void* mapMemory();
        void bind(const vk::raii::Buffer& buffer) const;
        void bind(const vk::raii::Image& image) const;

        DeviceMemoryInfo memoryInfo{};
    };

    class DeviceMemoryAllocatorBase
    {
    public:
        DeviceMemoryAllocatorBase() = default;
        virtual ~DeviceMemoryAllocatorBase() noexcept = default;

        DeviceMemoryAllocatorBase(const DeviceMemoryAllocatorBase&) = delete;
        DeviceMemoryAllocatorBase& operator=(const DeviceMemoryAllocatorBase&) = delete;
        DeviceMemoryAllocatorBase(DeviceMemoryAllocatorBase&&) noexcept = default;
        DeviceMemoryAllocatorBase& operator=(DeviceMemoryAllocatorBase&&) noexcept = default;

        virtual std::unique_ptr<DeviceMemoryBase> allocateMemory(const vk::MemoryRequirements& requirements) = 0;
        virtual void freeMemory(DeviceMemoryBase& memory) = 0;
    };

    class DeviceMemoryBlockBase
    {
    public:
        DeviceMemoryBlockBase(vk::raii::DeviceMemory&& memory_, DeviceMemoryAllocatorBase& allocator);
        virtual ~DeviceMemoryBlockBase();

        DeviceMemoryBlockBase(DeviceMemoryBlockBase&&) noexcept = default;
        DeviceMemoryBlockBase& operator=(DeviceMemoryBlockBase&&) noexcept = default;

        inline operator vk::DeviceMemory() const { return memory; }

        [[ nodiscard ]] void* mapMemory( vk::DeviceSize offset, vk::DeviceSize size,  vk::MemoryMapFlags flags = {} );

    protected:
        vk::raii::DeviceMemory memory{ nullptr };
        void* data = nullptr;
    };

    class DefaultDeviceMemoryAllocator : public DeviceMemoryAllocatorBase
    {
    public:
        explicit DefaultDeviceMemoryAllocator(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
            const std::function<bool(vk::MemoryPropertyFlags, vk::MemoryHeap)>& filterFunction = nullptr);
        explicit DefaultDeviceMemoryAllocator(const Device& device, const std::function<bool(vk::MemoryPropertyFlags, vk::MemoryHeap)>& filterFunction = nullptr);
    
        std::unique_ptr<DeviceMemoryBase> allocateMemory(const vk::MemoryRequirements& requirements);
        void freeMemory(DeviceMemoryBase& memory);

    private:
        const vk::raii::Device* pDevice = nullptr;
        uint32_t validMemoryIndex = UINT32_MAX;
    };

    template<class T = DeviceMemoryAllocatorBase>
    class MappableAllocator;

    template<>
    class MappableAllocator<DeviceMemoryAllocatorBase> : public DeviceMemoryAllocatorBase
    {
    public:
        template<class ... Args>
        MappableAllocator(const vk::raii::PhysicalDevice& physicalDevice, Args&& ... args) requires std::constructible_from<MappableAllocator, Args...>
            : MappableAllocator(physicalDevice, std::make_unique<MappableAllocator>(std::forward<Args>(args)...)) {}

        explicit MappableAllocator(const vk::raii::PhysicalDevice& physicalDevice, std::unique_ptr<DeviceMemoryAllocatorBase> && allocator_);

        MappableAllocator(const MappableAllocator&) = delete;
        MappableAllocator& operator=(const MappableAllocator&) = delete;
        MappableAllocator(MappableAllocator&&) = default;
        MappableAllocator& operator=(MappableAllocator&&) = default;
        
        inline DeviceMemoryAllocatorBase* operator->() & { return allocator.get(); }
        
        std::unique_ptr<DeviceMemoryBase> allocateMemory(const vk::MemoryRequirements& requirements);
        void freeMemory(DeviceMemoryBase& memory);

    protected:
        std::unique_ptr<DeviceMemoryAllocatorBase> allocator = nullptr;
        uint32_t mappableMemoryIndex = 0;
    };

    template<class T>
        requires (!std::is_reference_v<T>) && std::is_base_of_v<DeviceMemoryAllocatorBase, T>
    class MappableAllocator<T> : public MappableAllocator<DeviceMemoryAllocatorBase>
    {
    public:
        template<class ... Args>
        MappableAllocator(const vk::raii::PhysicalDevice& physicalDevice, Args&& ... args) requires std::constructible_from<T, Args...>
            : MappableAllocator<DeviceMemoryAllocatorBase>(physicalDevice, std::make_unique<T>(std::forward<Args>(args)...)) {}

        inline operator T&() & { return *allocator; }
        inline T* operator->() & { return allocator.get(); }
    };
}