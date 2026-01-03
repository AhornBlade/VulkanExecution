#pragma once

#include "Base.hpp"

namespace vke{

    template<class T>
    struct DeviceMemoryInfo
    {
        const vk::raii::DeviceMemory* memory = nullptr;
        uint32_t memoryIndex = 0;
        vk::DeviceSize offset = 0;
        T* mapped = nullptr;

        inline operator T*() noexcept{ return reinterpret_cast<T*>(mapped); }
    };
    
    class DeviceMemoryResource
    {
    public:
        explicit DeviceMemoryResource() = default;
        virtual ~DeviceMemoryResource() noexcept = default;

        DeviceMemoryResource(const DeviceMemoryResource&) = default;
        DeviceMemoryResource& operator=(const DeviceMemoryResource&) = default;
        DeviceMemoryResource(DeviceMemoryResource&&) noexcept = default;
        DeviceMemoryResource& operator=(DeviceMemoryResource&&) noexcept = default;

        DeviceMemoryInfo<void> allocate(vk::MemoryRequirements requirements);
        void deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements);
        bool is_equal(const DeviceMemoryResource& other) const noexcept;

        inline bool operator==(const DeviceMemoryResource& other) { return (this == &other) || is_equal(other); }

    private:
        virtual DeviceMemoryInfo<void> do_allocate(vk::MemoryRequirements requirements) = 0;
        virtual void do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements) = 0;
        virtual bool do_is_equal(const DeviceMemoryResource& other) const noexcept = 0;
    };

    class NewDeleteDeviceMemoryResource : public DeviceMemoryResource
    {
    public:
        NewDeleteDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
            std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter = nullptr);
        explicit NewDeleteDeviceMemoryResource(const Device& device, 
            std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter = nullptr);

    private:
        const vk::raii::Device* p_device = nullptr;
        vk::raii::PhysicalDevice physicalDevice_{nullptr};
        uint32_t memoryTypes = UINT32_MAX;

        DeviceMemoryInfo<void> do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    class DeviceLocalMemoryResource : public DeviceMemoryResource
    {
    public:
        DeviceLocalMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream);

        DeviceLocalMemoryResource(const DeviceLocalMemoryResource&) = default;
        DeviceLocalMemoryResource& operator=(const DeviceLocalMemoryResource&) = default;
        DeviceLocalMemoryResource(DeviceLocalMemoryResource&&) noexcept = default;
        DeviceLocalMemoryResource& operator=(DeviceLocalMemoryResource&&) noexcept = default;

    private:
        DeviceMemoryResource* p_resource = nullptr;
        uint32_t deviceLocalIndices = 0;

        DeviceMemoryInfo<void> do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    class MappedDeviceMemoryResource : public DeviceMemoryResource
    {
    public:
        MappedDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream);

        MappedDeviceMemoryResource(const MappedDeviceMemoryResource&) = default;
        MappedDeviceMemoryResource& operator=(const MappedDeviceMemoryResource&) = default;
        MappedDeviceMemoryResource(MappedDeviceMemoryResource&&) noexcept = default;
        MappedDeviceMemoryResource& operator=(MappedDeviceMemoryResource&&) noexcept = default;

    private:
        DeviceMemoryResource* p_resource = nullptr;
        uint32_t visibleIndices = 0;
        uint32_t coherentIndices = 0;

        DeviceMemoryInfo<void> do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    template<class T = void>
    class DeviceMemoryAllocator;
    
    template<>
    class DeviceMemoryAllocator<void>
    {
    public:
        DeviceMemoryAllocator() noexcept = default;
        DeviceMemoryAllocator(DeviceMemoryResource& resource) : p_resource{&resource} {};

        DeviceMemoryAllocator(const DeviceMemoryAllocator&) = default;
        DeviceMemoryAllocator& operator=(const DeviceMemoryAllocator&) = default;
        DeviceMemoryAllocator(DeviceMemoryAllocator&&) noexcept = default;
        DeviceMemoryAllocator& operator=(DeviceMemoryAllocator&&) noexcept = default;

        DeviceMemoryInfo<void> allocate_bytes(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::MemoryRequirements requirements);
        void deallocate_bytes(DeviceMemoryInfo<void> memory, vk::MemoryRequirements requirements);

        inline operator bool() const noexcept { return p_resource; }

    private:
        DeviceMemoryResource* p_resource = nullptr;
    };

    DeviceMemoryResource* getDefaultDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
    DeviceMemoryResource* setDefaultDeviceMemoryResource(DeviceMemoryResource* resource);
}