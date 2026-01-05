#pragma once

#include "Base.hpp"
#include "Memory.hpp"

namespace vke{
    class Swapchain
    {
    public:
        struct CreateInfo
        {
            Getter<vk::SurfaceKHR> surface;
            Selecter<vk::SurfaceFormatKHR> surfaceFormatSelecter{nullptr};
            Selecter<vk::PresentModeKHR> presentModeSelecter{nullptr}; 
            Getter<vk::Extent2D> imageExtent;
            Getter<uint32_t> imageCount{0};
            Getter<uint32_t> imageArrayLayers{1};
            Checker<vk::ImageUsageFlags> imageUsageChecker;
            Selecter<vk::SurfaceTransformFlagsKHR> surfaceTransformSelecter{nullptr};
            Selecter<vk::CompositeAlphaFlagsKHR> compositeAlphaSelecter{ nullptr, vk::CompositeAlphaFlagBitsKHR::eOpaque };
            Getter<std::vector<uint32_t>> queueFamilyIndices{std::vector<uint32_t>{}};
            Getter<vk::Bool32> clipped{vk::True};
        };
 
        Swapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, CreateInfo&& createInfo, 
            vk::SwapchainKHR oldSwapchain = {nullptr});
        Swapchain(const Device& device, CreateInfo&& createInfo, vk::SwapchainKHR oldSwapchain = {nullptr});
        
        Swapchain(Swapchain&&) noexcept = default;
        Swapchain& operator=(Swapchain&&) noexcept = default;

        struct ViewCreateInfo
        {
            struct Span
            {
                uint32_t base = 0;
                uint32_t count = 0;
            };

            Getter<vk::ImageViewCreateFlags> flags{ vk::ImageViewCreateFlags{} };
            Getter<vk::Format> format{ nullptr };
            Getter<vk::ComponentMapping> components{ vk::ComponentMapping{} };
            Getter<vk::ImageAspectFlags> aspectMask;
            Getter<Span(uint32_t arrayLayers)> getArrayLayersFunction = nullptr;
        };

        vk::raii::ImageView createImageView(const vk::raii::Device& device, const ViewCreateInfo& createInfo, uint32_t imageIndex) const;

        void recreate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
        void recreate(const Device& device);

        inline vk::Format getFormat() const noexcept { return nativeCreateInfo.imageFormat; }
        inline uint32_t getImageCount() const noexcept { return images.size(); }

        inline operator const vk::raii::SwapchainKHR & () const & noexcept { return swapchain; }
        inline operator vk::SwapchainKHR () const & noexcept { return swapchain; }
        inline const auto* operator->() const & noexcept { return &swapchain; }

    private:
        vk::SwapchainCreateInfoKHR nativeCreateInfo{};
        CreateInfo createInfo;
        vk::raii::SwapchainKHR swapchain{ nullptr };
        std::vector<vk::Image> images;

        vk::raii::SwapchainKHR createSwapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, vk::SwapchainKHR oldSwapchain);
    };

    class BufferWrapper
    {
    public:
        struct CreateInfo
        {
            Getter<vk::BufferCreateFlags> flags{ static_cast<vk::BufferCreateFlags>(0) };
            Getter<vk::DeviceSize> size;
            Getter<vk::BufferUsageFlags> usage;
            Getter<std::vector<uint32_t>> queueFamilyIndices{std::vector<uint32_t>{}};
        };
        explicit BufferWrapper() = default;
        BufferWrapper(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const CreateInfo& createInfo);
        
        BufferWrapper(BufferWrapper&&) noexcept = default;
        BufferWrapper& operator=(BufferWrapper&&) noexcept = default;

        vk::raii::Buffer buffer{ nullptr };
    };
    
    template<class T = void>
    class Buffer
    {
    public:
        struct CreateInfo
        {
            Getter<vk::BufferCreateFlags> flags{ static_cast<vk::BufferCreateFlags>(0) };
            Getter<std::vector<T>> data;
            Getter<vk::BufferUsageFlags> usage;
            Getter<std::vector<uint32_t>> queueFamilyIndices{std::vector<uint32_t>{}};
        };
        
        Buffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const BufferWrapper::CreateInfo& createInfo, 
            DeviceMemoryAllocator<T> deviceMemoryAllocator = DeviceMemoryAllocator<T>{})
            : buffer{device, physicalDevice, createInfo}
        {
            memory_ = deviceMemoryAllocator.allocate(device, physicalDevice, buffer.buffer.getMemoryRequirements());
            memory_.bind(buffer);
        }
        Buffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, const CreateInfo& createInfo, 
            DeviceMemoryAllocator<T> deviceMemoryAllocator = DeviceMemoryAllocator<T>{})
        {
            auto data = createInfo.data();
            buffer = BufferWrapper{device, physicalDevice, BufferWrapper::CreateInfo
                {
                    .flags = createInfo.flags(),
                    .size = data.size() * sizeof(T),
                    .usage = createInfo.usage(),
                    .queueFamilyIndices = createInfo.queueFamilyIndices()
                }};
            memory_ = deviceMemoryAllocator.allocate(device, physicalDevice, buffer.buffer.getMemoryRequirements());
            memory_.bind(buffer.buffer);
            std::ranges::copy(data, memory_.data());
        }
        Buffer(const Device& device, const BufferWrapper::CreateInfo& createInfo, DeviceMemoryAllocator<T> deviceMemoryAllocator = DeviceMemoryAllocator<T>{})
            : Buffer{device, device.getPhysicalDevice(), createInfo, deviceMemoryAllocator} {}
        Buffer(const Device& device, const CreateInfo& createInfo, DeviceMemoryAllocator<T> deviceMemoryAllocator = DeviceMemoryAllocator<T>{})
            : Buffer{device, device.getPhysicalDevice(), createInfo, deviceMemoryAllocator} {}
        
        Buffer(Buffer&&) noexcept = default;
        Buffer& operator=(Buffer&&) noexcept = default;

        inline operator const vk::raii::Buffer & () const & noexcept { return buffer.buffer; }
        inline operator vk::Buffer () const & noexcept { return buffer.buffer; }
        inline const auto* operator->() const & noexcept { return &buffer.buffer; }

    private:
        BufferWrapper buffer{};
        DeviceMemory<T> memory_{};
    };
    
    class Image
    {
    public:
        struct CreateInfo
        {
            Getter<vk::ImageCreateFlags> flags{ vk::ImageCreateFlags{} };
            Getter<vk::ImageTiling> tiling{ vk::ImageTiling::eOptimal };
            Getter<vk::ImageUsageFlags> usage;
            Getter<vk::ImageLayout> initialLayout{ vk::ImageLayout::eUndefined };
            Getter<vk::FormatFeatureFlags> formatFeatureFlags;
            Selecter<vk::Format> formatSelecter{ nullptr };
            Getter<vk::Extent3D> extent;
            Getter<uint32_t> mipLevels{ 1 };
            Getter<uint32_t> arrayLayers{ 1 };
            Selecter<vk::SampleCountFlags> sampleSelecter{ nullptr, vk::SampleCountFlagBits::e1 };
            Getter<std::vector<uint32_t>> queueFamilyIndices{std::vector<uint32_t>{}};
        };
        
        Image(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, CreateInfo&& createInfo, 
            DeviceMemoryAllocator<> deviceMemoryAllocator = DeviceMemoryAllocator<>{});
        Image(const Device& device, CreateInfo&& createInfo, DeviceMemoryAllocator<> deviceMemoryAllocator = DeviceMemoryAllocator<>{});
        
        Image(Image&&) noexcept = default;
        Image& operator=(Image&&) noexcept = default;

        struct ViewCreateInfo
        {
            struct Span
            {
                uint32_t base = 0;
                uint32_t count = 0;
            };

            Getter<vk::ImageViewCreateFlags> flags{ vk::ImageViewCreateFlags{} };
            Getter<vk::ImageViewType> viewType{ nullptr };
            Getter<vk::Format> format{ nullptr };
            Getter<vk::ComponentMapping> components{ vk::ComponentMapping{} };
            Getter<vk::ImageAspectFlags> aspectMask;
            Getter<Span(uint32_t mipLevels)> getMipLevelFunction = nullptr;
            Getter<Span(uint32_t arrayLayers)> getArrayLayersFunction = nullptr;
        };

        vk::raii::ImageView createImageView(const vk::raii::Device& device, const ViewCreateInfo& createInfo) const;

        inline operator const vk::raii::Image & () const & noexcept { return image; }
        inline operator vk::Image () const & noexcept { return image; }
        inline const auto* operator->() const & noexcept { return &image; }

        inline vk::Format getFormat() const noexcept { return nativeCreateInfo.format; }
        inline vk::SampleCountFlagBits getSamples() const noexcept { return nativeCreateInfo.samples; }
        inline vk::ImageLayout getInitialLayout() const noexcept { return nativeCreateInfo.initialLayout; }

        void recreate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, DeviceMemoryAllocator<> deviceMemoryAllocator = DeviceMemoryAllocator<>{});
        void recreate(const Device& device, DeviceMemoryAllocator<> deviceMemoryAllocator = DeviceMemoryAllocator<>{});

    private:
        vk::ImageCreateInfo nativeCreateInfo{};
        CreateInfo createInfo;
        vk::raii::Image image{ nullptr };
        DeviceMemory<void> memory_{};

        vk::raii::Image createImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice);
    };
}