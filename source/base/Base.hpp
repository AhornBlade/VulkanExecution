#pragma once

#include "Common.hpp"

namespace vke{

    class Instance
    {
    public:
        struct CreateInfo
        {
            const char* applicationName = "";
            uint32_t applicationVersion = VK_MAKE_VERSION(0, 0, 0);
            Checker<vk::ExtensionProperties> enabledExtensionChecker{false};
            Checker<vk::LayerProperties> enabledLayerChecker{false};
        };

        explicit Instance() = default;
        explicit Instance(const CreateInfo& createInfo);

        Instance(Instance&&) noexcept = default;
        Instance& operator=(Instance&&) noexcept = default;

        inline operator const vk::raii::Instance& () const & noexcept { return instance; }
        inline operator vk::Instance () const & noexcept { return instance; }
        inline const auto& operator->() const & noexcept { return instance; }

    private:
        vk::raii::Context context{};
        vk::raii::Instance instance{nullptr};
#ifdef _DEBUG
        vk::raii::DebugUtilsMessengerEXT debugMessenger{nullptr};
#endif
    };
    
    struct DeviceQueueInfo
    {
        template<typename QueueUsageFlagType>
        DeviceQueueInfo(QueueUsageFlagType usage, float priority)
            :queueUsageFlags(static_cast<uint32_t>(usage)), queuePriority(priority) {}

        uint32_t queueUsageFlags;
        float queuePriority;
    };

    class DeviceQueue
    {
    public:
        DeviceQueue(const vk::raii::Device& device, uint32_t queueFamilyIndex, uint32_t queueIndex);
        
        inline operator const vk::raii::Queue&() const & noexcept { return queue; }
        inline operator vk::Queue () const & noexcept { return queue; }
        inline const auto* operator->() const & noexcept { return &queue; }

        inline uint32_t getQueueFamilyIndex() const noexcept { return queueFamilyIndex; }
        inline uint32_t getQueueIndex() const noexcept { return queueIndex; }

    private:
        uint32_t queueFamilyIndex = 0;
        uint32_t queueIndex = 0;
        vk::raii::Queue queue{ nullptr };
    };

    class Device
    {
    public:
        struct CreateInfo
        {
            Selecter<vk::raii::PhysicalDevice> physicalDevicSelecter;
            Getter<std::vector<std::vector<DeviceQueueInfo>>(const vk::raii::PhysicalDevice&)> deviceQueueInfos{std::vector<std::vector<DeviceQueueInfo>>{ {{0xffffffff, 1.0f}} }};
            Checker<vk::ExtensionProperties> enabledExtensionChecker{false};
            Getter<vk::PhysicalDeviceFeatures(const vk::PhysicalDeviceFeatures&)> enabledFeaturesTransformer{ vk::PhysicalDeviceFeatures{} };
        };
        explicit Device() = default;
        explicit Device(const vk::raii::Instance& instance, const CreateInfo& createInfo);
        
        Device(Device&&) noexcept = default;
        Device& operator=(Device&&) noexcept = default;

        inline operator const vk::raii::Device & () const & noexcept { return *device; }
        inline operator vk::Device () const & noexcept { return *device; }
        inline const auto* operator->() const & noexcept { return device.get(); }

        inline const vk::raii::PhysicalDevice& getPhysicalDevice() const & noexcept { return physicalDevice; }

        const DeviceQueue& getDeviceQueue(const std::function<uint32_t(const DeviceQueueInfo&)>& queueEvaluationFunction) const &;

    private:
        vk::raii::PhysicalDevice physicalDevice{nullptr};
        std::vector<std::vector<DeviceQueueInfo>> deviceQueueInfos;
        std::vector<std::vector<DeviceQueue>> deviceQueues;
        std::unique_ptr<vk::raii::Device> device{nullptr};
    };

}