#pragma once

#include "Base.hpp"

#include <unordered_map>

namespace vke{

    struct DeviceMemoryInfo
    {
        const vk::raii::DeviceMemory* memory = nullptr;
        uint32_t memoryIndex = 0;
        vk::DeviceSize offset = 0;
        vk::DeviceSize size = 0;
        void* mapped = nullptr;
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

        DeviceMemoryInfo allocate(vk::MemoryRequirements requirements);
        void deallocate(DeviceMemoryInfo memory);
        bool is_equal(const DeviceMemoryResource& other) const noexcept;

        inline bool operator==(const DeviceMemoryResource& other) { return (this == &other) || is_equal(other); }

    private:
        virtual DeviceMemoryInfo do_allocate(vk::MemoryRequirements requirements) = 0;
        virtual void do_deallocate(DeviceMemoryInfo memory) = 0;
        virtual bool do_is_equal(const DeviceMemoryResource& other) const noexcept = 0;
    };

    class NewDeleteDeviceMemoryResource : public DeviceMemoryResource
    {
    public:
        NewDeleteDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
        explicit NewDeleteDeviceMemoryResource(const Device& device);

    private:
        const vk::raii::Device* p_device = nullptr;
        vk::raii::PhysicalDevice physicalDevice_{nullptr};

        DeviceMemoryInfo do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo memory) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    class FilterDeviceMemoryResource : public DeviceMemoryResource
    {
    public:
        explicit FilterDeviceMemoryResource() = default;
        FilterDeviceMemoryResource(
            const vk::raii::PhysicalDevice& physicalDevice, 
            DeviceMemoryResource* upstream,
            std::function<bool(vk::MemoryPropertyFlags property, vk::MemoryHeap heap)> memoryTypeFlilter);
            
        FilterDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream, vk::MemoryPropertyFlags required);

        FilterDeviceMemoryResource(const FilterDeviceMemoryResource&) = default;
        FilterDeviceMemoryResource& operator=(const FilterDeviceMemoryResource&) = default;
        FilterDeviceMemoryResource(FilterDeviceMemoryResource&&) noexcept = default;
        FilterDeviceMemoryResource& operator=(FilterDeviceMemoryResource&&) noexcept = default;

    private:
        DeviceMemoryResource* p_resource = nullptr;
        uint32_t validIndices = UINT32_MAX;

        DeviceMemoryInfo do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo memory) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    class MappedDeviceMemoryResource : public DeviceMemoryResource
    {
    public:
        explicit MappedDeviceMemoryResource() = default;
        MappedDeviceMemoryResource(const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream);

        MappedDeviceMemoryResource(const MappedDeviceMemoryResource&) = default;
        MappedDeviceMemoryResource& operator=(const MappedDeviceMemoryResource&) = default;
        MappedDeviceMemoryResource(MappedDeviceMemoryResource&&) noexcept = default;
        MappedDeviceMemoryResource& operator=(MappedDeviceMemoryResource&&) noexcept = default;

    private:
        DeviceMemoryResource* p_resource = nullptr;
        uint32_t visibleIndices = 0;
        uint32_t coherentIndices = 0;

        DeviceMemoryInfo do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo memory) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };

    class QueueTransferMemoryResource : public DeviceMemoryResource
    {
    public:
        explicit QueueTransferMemoryResource() = default;
        QueueTransferMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* deviceLocalUpstream,
            DeviceMemoryResource* stagingUpstream);
        QueueTransferMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryResource* upstream);
        QueueTransferMemoryResource(const Device& device, DeviceMemoryResource* deviceLocalUpstream, DeviceMemoryResource* stagingUpstream);
        QueueTransferMemoryResource(const Device& device, DeviceMemoryResource* upstream);

        QueueTransferMemoryResource(const QueueTransferMemoryResource&) = delete;
        QueueTransferMemoryResource& operator=(const QueueTransferMemoryResource&) = delete;
        QueueTransferMemoryResource(QueueTransferMemoryResource&&) noexcept = default;
        QueueTransferMemoryResource& operator=(QueueTransferMemoryResource&&) noexcept = default;

        void cmdUpload(const vk::raii::CommandBuffer& commandBuffer) const;

    private:
        const vk::raii::Device* p_device = nullptr;
        FilterDeviceMemoryResource deviceLocal{};
        FilterDeviceMemoryResource coherent{};
        MappedDeviceMemoryResource staging{};

        struct UploadDeviceMemory
        {
            DeviceMemoryInfo memory;
            vk::raii::Buffer deviceLocal = nullptr;
            vk::raii::Buffer staging = nullptr;
        };

        std::unordered_map< void*, UploadDeviceMemory > map{};
        
        DeviceMemoryInfo do_allocate(vk::MemoryRequirements requirements) override;
        void do_deallocate(DeviceMemoryInfo memory) override;
        bool do_is_equal(const DeviceMemoryResource& other) const noexcept override;
    };
    
    DeviceMemoryResource* getDefaultDeviceMemoryResource(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
    DeviceMemoryResource* setDefaultDeviceMemoryResource(DeviceMemoryResource* resource);

    template<class T>
    class DeviceMemory
    {
    public:
        explicit DeviceMemory() = default;
        DeviceMemory(DeviceMemoryInfo info, std::function<void(DeviceMemoryInfo*)> deleter) : info_{info}, deleter_{deleter} {};
        ~DeviceMemory() { if(deleter_) { deleter_(&info_); } };

        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory& operator=(const DeviceMemory&) = delete;
        DeviceMemory(DeviceMemory&&) noexcept = default;
        DeviceMemory& operator=(DeviceMemory&&) noexcept = default;

        inline void bind(const vk::raii::Buffer& buffer) { buffer.bindMemory(*(info_.memory), info_.offset); }
        inline void bind(const vk::raii::Image& image) { image.bindMemory(*(info_.memory), info_.offset); }

        inline T* data() noexcept { return reinterpret_cast<T*>(info_.mapped); }
        inline size_t size() noexcept { return info_.size / sizeof(T) ; }

    private:
        DeviceMemoryInfo info_{};
        std::function<void(DeviceMemoryInfo*)> deleter_ = nullptr;
    };
    
    template<class T = void>
    class DeviceMemoryAllocator
    {
    public:
        DeviceMemoryAllocator() noexcept = default;
        DeviceMemoryAllocator(DeviceMemoryResource& resource) : p_resource{&resource} {};

        DeviceMemoryAllocator(const DeviceMemoryAllocator&) = default;
        DeviceMemoryAllocator& operator=(const DeviceMemoryAllocator&) = default;
        DeviceMemoryAllocator(DeviceMemoryAllocator&&) noexcept = default;
        DeviceMemoryAllocator& operator=(DeviceMemoryAllocator&&) noexcept = default;

        inline DeviceMemory<T> allocate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::MemoryRequirements requirements)
        {
            if(!p_resource) p_resource = getDefaultDeviceMemoryResource(device, physicalDevice);
            return {p_resource->allocate(requirements), 
                [p_resource = this->p_resource](DeviceMemoryInfo* info) { p_resource->deallocate(*info); }};
        }

        inline operator bool() const noexcept { return p_resource; }

    private:
        DeviceMemoryResource* p_resource = nullptr;
    };
}