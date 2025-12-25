#pragma once

#include "Common.hpp"

#include <concepts>
#include <coroutine>

namespace vke{

    class SyncPool
    {
    public:
        SyncPool(const vk::raii::Device& device, uint32_t fenceCount, uint32_t semaphoreCount);

    private:
        std::vector<vk::raii::Fence> fences;
        std::vector<vk::raii::Semaphore> semaphores;
    };

    template<class Fence>
        requires std::same_as<std::remove_cvref_t<Fence>, vk::raii::Fence>
    auto operator co_await( Fence&& fence )
    {
        struct awaitable
        {
            Fence fence;
            bool await_ready() { return fence.getStatus() == vk::Result::eSuccess; }
            void await_suspend(std::coroutine_handle<> h) { }
            void await_resume() 
            {
                fence.getDispatcher()->vkWaitForFences(fence.getDevice(), 1, reinterpret_cast<const VkFence*>(&(*fence)), vk::True, UINT64_MAX);
            }
        };

        return awaitable{std::forward<Fence>(fence)};
    }

}