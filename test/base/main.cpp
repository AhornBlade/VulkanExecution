#include <vulkan_execution.hpp>
#include <vulkan/vulkan_format_traits.hpp>

#include <extension/glfw.hpp>

#include <ranges>
#include <iostream>

#include <glm/glm.hpp>

enum QueueUsageFlagBits
{
    eGraphics = 0x01,
    ePresent = 0x02
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class HelloTriangleApplication
{
public:

private:
    vke::GLFWInstance glfwInstance{};
    vke::Instance instance{vke::Instance::CreateInfo{ 
        .applicationName="test_base", 
        .applicationVersion=VK_MAKE_VERSION(1, 0, 0), 
        .enabledExtensionChecker{
            [exts = glfwInstance.getInstanceExtensions()](const vk::ExtensionProperties& p) -> bool
            {
                return std::ranges::find_if(exts, [&](const char* str){ return std::strcmp(str, p.extensionName) == 0; }) != std::ranges::end(exts);
            }
        }}};
    vke::GLFWWindow window{glfwInstance, instance, {
        .width = 800, .height = 600, .title = "test_base", 
        .triggerSetFramebufferSize = [this](uint32_t w, uint32_t h){ triggerSetFramebufferSize({w, h}); } } };
    vke::Device device = vke::Device{instance, vke::Device::CreateInfo{ 
        .physicalDevicSelecter{[this](const vk::raii::PhysicalDevice& p) -> uint32_t
        {
            auto properties = p.getProperties();

            uint32_t score = 0;

            switch(properties.deviceType)
            {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                score += 50;
                break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                score += 20;
                break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                score += 10;
                break;
            default:
                break;
            }

            return score;
        }, [this](const vk::raii::PhysicalDevice& p) -> bool
        {
            return std::ranges::includes(p.enumerateDeviceExtensionProperties() | std::views::transform([](vk::ExtensionProperties eps){ return eps.extensionName; }), 
                glfwInstance.getDeviceExtensions(), std::strcmp);
        } },
        .deviceQueueInfos = [this](const vk::raii::PhysicalDevice& p)
        {
            auto queueProperties = p.getQueueFamilyProperties();

            std::vector<std::vector<vke::DeviceQueueInfo>> deviceQueueInfos(queueProperties.size());

            uint32_t graphicsFamilies = 0, presentFamilies = 0;

            for(const auto& [index, queueFamily] : queueProperties | std::views::enumerate)
            {
                if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) 
                {
                    graphicsFamilies |= ( 1 << index );
                }
                
                if (p.getSurfaceSupportKHR(index, window.getSurface())) 
                {
                    presentFamilies |= ( 1 << index );
                }
            }

            if(graphicsFamilies == 0 || presentFamilies == 0)
            {
                throw std::runtime_error{" Failed to find suitable queues"};
            }

            if(graphicsFamilies & presentFamilies)
            {
                for(uint32_t index = 0; index < 32; index++)
                {
                    if(graphicsFamilies & presentFamilies & ( 1 << index ))
                    {
                        deviceQueueInfos[index].emplace_back(QueueUsageFlagBits::eGraphics | QueueUsageFlagBits::ePresent, 1.0f);
                    }
                }
            }
            else
            {
                uint32_t graphicsFamily = UINT32_MAX, presentFamily = UINT32_MAX;
                for(uint32_t index = 0; index < 32; index++)
                {
                    if((graphicsFamilies & ( 1 << index )) && graphicsFamily == UINT32_MAX)
                    {
                        graphicsFamily = index;
                        deviceQueueInfos[index].emplace_back(QueueUsageFlagBits::eGraphics, 1.0f);
                    }

                    if((presentFamilies & ( 1 << index )) && presentFamily == UINT32_MAX)
                    {
                        presentFamily = index;
                        deviceQueueInfos[index].emplace_back(QueueUsageFlagBits::ePresent, 1.0f);
                    }

                    if(graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX)
                        break;
                }
            }

            return deviceQueueInfos;
        },
        .enabledExtensionChecker{
            [exts = glfwInstance.getDeviceExtensions()](const vk::ExtensionProperties& p) -> bool
            {
                return std::ranges::find_if(exts, [&](const char* str){ return std::strcmp(str, p.extensionName) == 0; }) != std::ranges::end(exts);
            }
        }}};
    
    const vke::DeviceQueue& graphicsQueue = device.getDeviceQueue(
        [](const vke::DeviceQueueInfo& queue) { return queue.queueUsageFlags | QueueUsageFlagBits::eGraphics; });
    const vke::DeviceQueue& presentQueue = device.getDeviceQueue(
        [](const vke::DeviceQueueInfo& queue) { return queue.queueUsageFlags | QueueUsageFlagBits::ePresent; });

    vke::NewDeleteDeviceMemoryResource memoryResource{device};
    vke::FilterDeviceMemoryResource deviceLocalMemory{device.getPhysicalDevice(), &memoryResource, vk::MemoryPropertyFlagBits::eDeviceLocal};
    vke::MappedDeviceMemoryResource mappedMemory{device.getPhysicalDevice(), &memoryResource};
    vke::QueueTransferMemoryResource transferMemory{device, &memoryResource};
    
    vke::Swapchain swapchain{device, vke::Swapchain::CreateInfo{
        .surface = window.getSurface(),
        .imageExtent{ [this]{ return window.getExtent2D(); } },
        .imageUsageChecker{vk::ImageUsageFlagBits::eColorAttachment},
        .queueFamilyIndices{std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex(), presentQueue.getQueueFamilyIndex()}}
    }};

    vke::Image depthImage{device, vke::Image::CreateInfo{
        .usage{vk::ImageUsageFlagBits::eDepthStencilAttachment},
        .formatFeatureFlags{vk::FormatFeatureFlagBits::eDepthStencilAttachment},
        .formatSelecter{nullptr, [](vk::Format format) 
        { 
            auto type = vk::componentNumericFormat(format, 0);
            return (std::strcmp( vk::componentName(format, 0), "D") == 0) && (
                (std::strcmp(type, "UINT") == 0) ||
                (std::strcmp(type, "SINT") == 0) ||
                (std::strcmp(type, "UFLOAT") == 0) ||
                (std::strcmp(type, "SFLOAT") == 0)
                ); 
        }},
        .extent{ [this] { return vk::Extent3D{window.getExtent2D(), 1}; } },
        .queueFamilyIndices{ std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex()} }
    }, deviceLocalMemory};
    
    vke::Image textureImage{device, vke::Image::CreateInfo{
        .usage{vk::ImageUsageFlagBits::eSampled},
        .formatFeatureFlags{vk::FormatFeatureFlagBits::eSampledImage},
        .formatSelecter{ nullptr, std::vector{vk::Format::eR8G8B8A8Srgb} },
        .extent{ [this] { return vk::Extent3D{window.getExtent2D(), 1}; } },
        .queueFamilyIndices{ std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex()} }
    }, deviceLocalMemory};
    
    vke::Buffer<Vertex> vertexBuffer{device, vke::Buffer<Vertex>::CreateInfo{
        .flags = static_cast<vk::BufferCreateFlags>(0),
        .data = std::vector<Vertex>(10),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer,
        .queueFamilyIndices{ std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex()} }
    }, transferMemory};
    
    vke::Buffer<uint32_t> indexBuffer{device, vke::Buffer<uint32_t>::CreateInfo{
        .flags = static_cast<vk::BufferCreateFlags>(0),
        .data = std::vector<uint32_t>(10),
        .usage = vk::BufferUsageFlagBits::eIndexBuffer,
        .queueFamilyIndices{ std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex()} }
    }, transferMemory};
    
    vke::Buffer<UniformBufferObject> uniformBuffer{device, vke::Buffer<UniformBufferObject>::CreateInfo{
        .flags = static_cast<vk::BufferCreateFlags>(0),
        .data = std::vector<UniformBufferObject>(1),
        .usage = vk::BufferUsageFlagBits::eUniformBuffer,
        .queueFamilyIndices{ std::vector<uint32_t>{graphicsQueue.getQueueFamilyIndex()} }
    }, mappedMemory};

    void triggerSetFramebufferSize(vk::Extent2D extent)
    {
        swapchain.recreate(device);
        depthImage.recreate(device, deviceLocalMemory);
    }
};

int main()
{
    HelloTriangleApplication app{};
}