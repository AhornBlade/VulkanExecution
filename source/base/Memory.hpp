#pragma once

#include "Base.hpp"

namespace vke{
    
    struct DeviceMemoryInfo
    {
        vk::DeviceMemory memory = nullptr;
        vk::DeviceSize offset = 0;
        vk::DeviceSize size = 0;
    };

    template<class State = void>
    class DeviceMemory;

    template<>
    class DeviceMemory<void>
    {
    public:
        explicit DeviceMemory() = default;
        virtual ~DeviceMemory() = default;

        DeviceMemory(const DeviceMemory&) = delete;
        DeviceMemory& operator=(const DeviceMemory&) = delete;
        DeviceMemory(DeviceMemory&&) noexcept = default;
        DeviceMemory& operator=(DeviceMemory&&) noexcept = default;

        inline void bind(const vk::raii::Buffer& buffer) const { auto info = getDeviceMemoryInfo(); buffer.bindMemory(info.memory, info.offset); };
        inline void bind(const vk::raii::Image& image) const { auto info = getDeviceMemoryInfo(); image.bindMemory(info.memory, info.offset); };
        inline operator vk::DeviceMemory() const { return getDeviceMemoryInfo().memory; }

        virtual DeviceMemoryInfo getDeviceMemoryInfo() const = 0;
    };

    template<class State>
        requires (!std::is_void_v<State>)
    class DeviceMemory<State> : public DeviceMemory<>
    {
        static_assert(std::same_as<State, std::remove_cvref_t<State>>);
        static_assert(requires(State s) { { s.getDeviceMemoryInfo() } -> std::same_as<DeviceMemoryInfo>; });
    public:
        explicit DeviceMemory(State&& state) : state_{ std::make_unique<State>(std::move(state)) } {}
        DeviceMemoryInfo getDeviceMemoryInfo() const override { return state_->getDeviceMemoryInfo(); }

    private:
        std::unique_ptr<State> state_;
    };

    template<class Strategy = void>
    class DeviceMemoryAllocator;

    template<>
    class DeviceMemoryAllocator<void>
    {
    public:
        explicit DeviceMemoryAllocator() = default;
        virtual ~DeviceMemoryAllocator() = default;

        DeviceMemoryAllocator(const DeviceMemoryAllocator&) = delete;
        DeviceMemoryAllocator& operator=(const DeviceMemoryAllocator&) = delete;
        DeviceMemoryAllocator(DeviceMemoryAllocator&&) noexcept = default;
        DeviceMemoryAllocator& operator=(DeviceMemoryAllocator&&) noexcept = default;

        virtual std::unique_ptr<DeviceMemory<>> allocateMemory(const vk::MemoryRequirements& requirements) = 0;
    };
    
    template<class Strategy>
        requires (!std::is_void_v<Strategy>)
    class DeviceMemoryAllocator<Strategy> : public DeviceMemoryAllocator<>
    {
        static_assert(std::same_as<Strategy, std::remove_cvref_t<Strategy>>);
    public:
        explicit DeviceMemoryAllocator(Strategy&& strategy) : strategy_{ std::make_unique<Strategy>(std::move(strategy)) } {}

        std::unique_ptr<DeviceMemory<>> allocateMemory(const vk::MemoryRequirements& requirements) override
        {
            return std::make_unique< DeviceMemory<typename Strategy::MemoryState> > (strategy_->allocateMemory( requirements ));
        }

    private:
        std::unique_ptr<Strategy> strategy_;
    };

    template<class State = void>
    class VisibleDeviceMemory;
    
    template<>
    class VisibleDeviceMemory<void> : public DeviceMemory<>
    {
    public:
        virtual void* data() const = 0;
        virtual size_t size() const = 0;
    };
    
    template<class State>
        requires (!std::is_void_v<State>)
    class VisibleDeviceMemory<State> : public VisibleDeviceMemory<>
    {
        static_assert(std::same_as<State, std::remove_cvref_t<State>>);
        static_assert(requires(State s) { { s.getDeviceMemoryInfo() } -> std::same_as<DeviceMemoryInfo>; });
        static_assert(requires(State s) { { s.data() } -> std::same_as<void*>; });
    public:
        explicit VisibleDeviceMemory(State&& state) : state_{ std::make_unique<State>(std::move(state)) } {}
        void* data() const override { return state_->data(); };
        size_t size() const override { return static_cast<size_t>(state_->getDeviceMemoryInfo().size); };
        
        DeviceMemoryInfo getDeviceMemoryInfo() const override { return state_->getDeviceMemoryInfo(); }

    protected:
        std::unique_ptr<State> state_;
    };
    
    template<class Strategy = void>
    class VisibleDeviceMemoryAllocator;
    
    template<>
    class VisibleDeviceMemoryAllocator<void>
    {
    public:
        explicit VisibleDeviceMemoryAllocator() = default;
        virtual ~VisibleDeviceMemoryAllocator() = default;

        VisibleDeviceMemoryAllocator(const VisibleDeviceMemoryAllocator&) = delete;
        VisibleDeviceMemoryAllocator& operator=(const VisibleDeviceMemoryAllocator&) = delete;
        VisibleDeviceMemoryAllocator(VisibleDeviceMemoryAllocator&&) noexcept = default;
        VisibleDeviceMemoryAllocator& operator=(VisibleDeviceMemoryAllocator&&) noexcept = default;

        virtual std::unique_ptr<VisibleDeviceMemory<>> allocateMemory(const vk::MemoryRequirements& requirements) = 0;
    };
    
    template<class Strategy>
        requires (!std::is_void_v<Strategy>)
    class VisibleDeviceMemoryAllocator<Strategy> : public VisibleDeviceMemoryAllocator<>
    {
        static_assert(std::same_as<Strategy, std::remove_cvref_t<Strategy>>);
    public:
        explicit VisibleDeviceMemoryAllocator(Strategy&& strategy) : strategy_{ std::make_unique<Strategy>(std::move(strategy)) } {}

        std::unique_ptr<VisibleDeviceMemory<>> allocateMemory(const vk::MemoryRequirements& requirements) override
        {
            return std::make_unique< VisibleDeviceMemory<typename Strategy::VisibleMemoryState> > (strategy_->allocateVisibleMemory( requirements ));
        }

    private:
        std::unique_ptr<Strategy> strategy_;
    };

    class DefaultDeviceMemoryStrategy
    {
    public:
        class MemoryState
        {
        public:
            MemoryState(const vk::raii::Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex);

            inline DeviceMemoryInfo getDeviceMemoryInfo() const { return DeviceMemoryInfo{memory_, 0, size_}; }

        protected:
            vk::raii::DeviceMemory memory_ = nullptr;
            vk::DeviceSize size_ = 0;
        };

        class VisibleMemoryState : public MemoryState
        {
        public:
            VisibleMemoryState(const vk::raii::Device& device, vk::DeviceSize size, uint32_t memoryTypeIndex);
            ~VisibleMemoryState();

            VisibleMemoryState(VisibleMemoryState&&) noexcept = default;
            VisibleMemoryState& operator=(VisibleMemoryState&&) noexcept = default;

            inline void* data() noexcept { return data_.get(); }

        private:
            std::unique_ptr<void, decltype([](void*){})> data_;
        };

        DefaultDeviceMemoryStrategy(const vk::raii::Device& device_) : device(&device_) {}

        MemoryState allocateMemory(const vk::MemoryRequirements& requirements) const;
        
        VisibleMemoryState allocateVisibleMemory(const vk::MemoryRequirements& requirements) const;

    private:
        const vk::raii::Device* device = nullptr;
    };
}