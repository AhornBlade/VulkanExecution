#include "Resources.hpp"

#include <vulkan/vulkan_format_traits.hpp>

#include <ranges>

namespace vke{

    Buffer::Buffer(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        const CreateInfo& createInfo_, DeviceMemoryAllocator<> deviceMemoryAllocator)
        : allocator_(deviceMemoryAllocator)
    {
        vk::BufferCreateInfo createInfo{};

        createInfo.setSize(createInfo_.size());
        createInfo.setUsage(createInfo_.usage());
        createInfo.setFlags(createInfo_.flags());

        std::vector<uint32_t> queueFamilyIndices = createInfo_.queueFamilyIndices();
        std::ranges::sort(queueFamilyIndices);
        auto [first, last] = std::ranges::unique(queueFamilyIndices);
        queueFamilyIndices.erase(first, last);
        
        createInfo.setSharingMode(queueFamilyIndices.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive);
        createInfo.setQueueFamilyIndices(queueFamilyIndices);

        buffer = vk::raii::Buffer{ device, createInfo};

        memory_ = deviceMemoryAllocator.allocate_bytes(device, physicalDevice, buffer.getMemoryRequirements());
        buffer.bindMemory(*memory_.memory, memory_.offset);
    }

    Buffer::Buffer(const Device& device, const CreateInfo& createInfo, DeviceMemoryAllocator<> deviceMemoryAllocator)
        : Buffer{ device, device.getPhysicalDevice(), createInfo, deviceMemoryAllocator } {}

    Buffer::~Buffer() noexcept
    {
        if(allocator_)
        {
            allocator_.deallocate_bytes(memory_, buffer.getMemoryRequirements());
        }
    }
    
    Swapchain::Swapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, CreateInfo&& createInfo_, vk::SwapchainKHR oldSwapchain)
        : createInfo{std::move(createInfo_)}, swapchain{createSwapchain(device, physicalDevice, oldSwapchain)}, images{swapchain.getImages()} {}

    Swapchain::Swapchain(const Device& device, CreateInfo&& createInfo, vk::SwapchainKHR oldSwapchain)
        : Swapchain{device, device.getPhysicalDevice(), std::move(createInfo), oldSwapchain } {}
    
    void Swapchain::recreate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
    {
        vk::raii::SwapchainKHR newSwapchain = createSwapchain(device, physicalDevice, swapchain);
        swapchain = std::move(newSwapchain);
        images = swapchain.getImages();
    }

    void Swapchain::recreate(const Device& device)
    {
        recreate(device, device.getPhysicalDevice());
    }
        
    vk::raii::SwapchainKHR Swapchain::createSwapchain(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        vk::SwapchainKHR oldSwapchain)
    {
        nativeCreateInfo.surface = createInfo.surface();
        vk::SurfaceFormatKHR surfaceFormat = createInfo.surfaceFormatSelecter(physicalDevice.getSurfaceFormatsKHR(nativeCreateInfo.surface));

        auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(nativeCreateInfo.surface);

        nativeCreateInfo.setImageFormat(surfaceFormat.format);
        nativeCreateInfo.setImageColorSpace(surfaceFormat.colorSpace);
        nativeCreateInfo.setPresentMode( createInfo.presentModeSelecter(physicalDevice.getSurfacePresentModesKHR(nativeCreateInfo.surface)) );
        vk::Extent2D extent = createInfo.imageExtent();
        nativeCreateInfo.setImageExtent({
            std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)});
        nativeCreateInfo.setMinImageCount(std::clamp(createInfo.imageCount(), capabilities.minImageCount, capabilities.maxImageCount));
        nativeCreateInfo.setImageArrayLayers(createInfo.imageArrayLayers());
        nativeCreateInfo.setImageUsage(createInfo.imageUsageChecker(capabilities.supportedUsageFlags));
        nativeCreateInfo.setPreTransform(createInfo.surfaceTransformSelecter(capabilities.supportedTransforms, capabilities.currentTransform));
        nativeCreateInfo.setCompositeAlpha( createInfo.compositeAlphaSelecter(capabilities.supportedCompositeAlpha) );
        nativeCreateInfo.setClipped(createInfo.clipped());

        auto queueFamilyIndices = createInfo.queueFamilyIndices();
        std::ranges::sort(queueFamilyIndices);
        auto [first, last] = std::ranges::unique(queueFamilyIndices);
        queueFamilyIndices.erase(first, last);
        
        nativeCreateInfo.setImageSharingMode(queueFamilyIndices.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive);
        nativeCreateInfo.setQueueFamilyIndices(queueFamilyIndices);

        nativeCreateInfo.setOldSwapchain(oldSwapchain);

        return vk::raii::SwapchainKHR{device, nativeCreateInfo};
    }
    
    Image::Image(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice, 
        CreateInfo&& createInfo_, DeviceMemoryAllocator<> deviceMemoryAllocator)
        : createInfo{ std::move(createInfo_) }, image{ createImage(device, physicalDevice) }, 
        allocator_{ deviceMemoryAllocator },
        memory_{ deviceMemoryAllocator.allocate_bytes(device, physicalDevice, image.getMemoryRequirements()) }
    {
        image.bindMemory(*memory_.memory, memory_.offset);
    }

    Image::Image(const Device& device, CreateInfo&& createInfo, DeviceMemoryAllocator<> deviceMemoryAllocator)
        : Image{device, device.getPhysicalDevice(), std::move(createInfo), deviceMemoryAllocator} { }
        
    Image::~Image() noexcept
    {
        if(allocator_)
        {
            allocator_.deallocate_bytes(memory_, image.getMemoryRequirements());
        }
    }
        
    void Image::recreate(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
    {
        image = createImage(device, physicalDevice);
        memory_ = allocator_.allocate_bytes(device, physicalDevice, image.getMemoryRequirements());
        image.bindMemory(*memory_.memory, memory_.offset);
    }

    void Image::recreate(const Device& device)
    {
        recreate(device, device.getPhysicalDevice());
    }
        
    vk::raii::Image Image::createImage(const vk::raii::Device& device, const vk::raii::PhysicalDevice& physicalDevice)
    {
        nativeCreateInfo.setFlags(createInfo.flags());
        nativeCreateInfo.setTiling(createInfo.tiling());
        nativeCreateInfo.setUsage(createInfo.usage());
        nativeCreateInfo.setInitialLayout(createInfo.initialLayout());
        nativeCreateInfo.setExtent(createInfo.extent());

        if (nativeCreateInfo.extent.height == 1)
            nativeCreateInfo.setImageType(vk::ImageType::e1D);
        else if (nativeCreateInfo.extent.depth == 1)
            nativeCreateInfo.setImageType(vk::ImageType::e2D);
        else
            nativeCreateInfo.setImageType(vk::ImageType::e3D);

        {
            vk::FormatFeatureFlags enabledFeature = createInfo.formatFeatureFlags();

            Selecter<vk::Format> selecter{createInfo.formatSelecter.f_, [&](vk::Format f) -> bool
            {
                if(!createInfo.formatSelecter.checker(f))
                    return false;

                auto properties = physicalDevice.getFormatProperties(f);
                vk::FormatFeatureFlags features = nativeCreateInfo.tiling == vk::ImageTiling::eLinear ? properties.linearTilingFeatures : properties.optimalTilingFeatures;

                vk::ImageFormatProperties p{};
                vk::Result r = static_cast<vk::Result>(physicalDevice.getDispatcher()->vkGetPhysicalDeviceImageFormatProperties( static_cast<VkPhysicalDevice>( *physicalDevice ),
                    static_cast<VkFormat>( f ),
                    static_cast<VkImageType>( nativeCreateInfo.imageType ),
                    static_cast<VkImageTiling>( nativeCreateInfo.tiling ),
                    static_cast<VkImageUsageFlags>( nativeCreateInfo.usage ),
                    static_cast<VkImageCreateFlags>( nativeCreateInfo.flags ),
                    reinterpret_cast<VkImageFormatProperties *>( &p ) ) );

                return (features & enabledFeature) == enabledFeature && r == vk::Result::eSuccess;
            }};

            nativeCreateInfo.setFormat(selecter(vk::getAllFormats()));
        }

        auto formatProperties = physicalDevice.getImageFormatProperties(nativeCreateInfo.format, 
            nativeCreateInfo.imageType, nativeCreateInfo.tiling, nativeCreateInfo.usage, nativeCreateInfo.flags);

        nativeCreateInfo.setArrayLayers(std::min(createInfo.arrayLayers(), formatProperties.maxArrayLayers));
        nativeCreateInfo.setMipLevels(std::min( createInfo.mipLevels(), formatProperties.maxMipLevels ));
        nativeCreateInfo.setSamples(createInfo.sampleSelecter(formatProperties.sampleCounts));

        std::vector<uint32_t> queueFamilyIndices = createInfo.queueFamilyIndices();
        std::ranges::sort(queueFamilyIndices);
        auto [first, last] = std::ranges::unique(queueFamilyIndices);
        queueFamilyIndices.erase(first, last);
        
        nativeCreateInfo.setSharingMode(queueFamilyIndices.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive);
        nativeCreateInfo.setQueueFamilyIndices(queueFamilyIndices);

        return vk::raii::Image{device, nativeCreateInfo};
    }

    vk::raii::ImageView Swapchain::createImageView(const vk::raii::Device& device, const ViewCreateInfo& createInfo_, uint32_t imageIndex) const
    {
        vk::ImageViewCreateInfo viewCreateInfo{};

        viewCreateInfo.setImage(images[imageIndex]);
        viewCreateInfo.setViewType(vk::ImageViewType::e2D);
        viewCreateInfo.setFlags(createInfo_.flags());
        viewCreateInfo.setFormat(createInfo_.format(nativeCreateInfo.imageFormat));
        viewCreateInfo.setComponents(createInfo_.components());
        viewCreateInfo.subresourceRange.setAspectMask(createInfo_.aspectMask());

        auto [baseLayer, layerCount] = createInfo_.getArrayLayersFunction(ViewCreateInfo::Span{0, nativeCreateInfo.imageArrayLayers}, 
            nativeCreateInfo.imageArrayLayers);
        viewCreateInfo.subresourceRange.setBaseMipLevel(0);
        viewCreateInfo.subresourceRange.setLevelCount(1);
        viewCreateInfo.subresourceRange.setBaseArrayLayer(baseLayer);
        viewCreateInfo.subresourceRange.setLayerCount(layerCount);

        return vk::raii::ImageView{device, viewCreateInfo};
    }
        
    vk::raii::ImageView Image::createImageView(const vk::raii::Device& device, const ViewCreateInfo& createInfo_) const
    {
        vk::ImageViewCreateInfo viewCreateInfo{};

        viewCreateInfo.setImage(image);
        viewCreateInfo.setViewType(createInfo_.viewType(static_cast<vk::ImageViewType>(nativeCreateInfo.imageType)));
        viewCreateInfo.setFlags(createInfo_.flags());
        viewCreateInfo.setFormat(createInfo_.format(nativeCreateInfo.format));
        viewCreateInfo.setComponents(createInfo_.components());
        viewCreateInfo.subresourceRange.setAspectMask(createInfo_.aspectMask());

        auto [baseLevel, levelCount] = createInfo_.getMipLevelFunction(ViewCreateInfo::Span{0, nativeCreateInfo.mipLevels}, 
            nativeCreateInfo.mipLevels);
        auto [baseLayer, layerCount] = createInfo_.getArrayLayersFunction(ViewCreateInfo::Span{0, nativeCreateInfo.arrayLayers},
            nativeCreateInfo.arrayLayers);
        viewCreateInfo.subresourceRange.setBaseMipLevel(baseLevel);
        viewCreateInfo.subresourceRange.setLevelCount(levelCount);
        viewCreateInfo.subresourceRange.setBaseArrayLayer(baseLayer);
        viewCreateInfo.subresourceRange.setLayerCount(layerCount);

        return vk::raii::ImageView{device, viewCreateInfo};
    }
}