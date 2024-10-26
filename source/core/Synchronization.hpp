#pragma once

#include "utils.hpp"
#include <mutex>

namespace vke{

    enum class time_status
    {
        no_timeout,
        timeout
    };

    class CountingSemaphore
    {
    public:
        [[ nodiscard ]]explicit CountingSemaphore() = default;
        [[ nodiscard ]]CountingSemaphore(const vk::raii::Device& device, uint64_t desired = 0);

        void release(uint64_t update = 1);
        [[ nodiscard ]] time_status acquire(uint64_t rel_time = UINT64_MAX);

        [[ nodiscard ]] inline vk::Semaphore get() const noexcept { return semaphore_; }
		[[ nodiscard ]] inline operator const vk::raii::Semaphore&() const noexcept { return semaphore_; }
		inline const vk::raii::Semaphore* operator->() const noexcept { return &semaphore_; }

    private:
        vk::raii::Semaphore semaphore_{VK_NULL_HANDLE};
    };

    class BinarySemaphore
    {
    public:
        [[ nodiscard ]]explicit BinarySemaphore() = default;
        [[ nodiscard ]]BinarySemaphore(const vk::raii::Device& device);

        [[ nodiscard ]] time_status acquire(uint64_t rel_time = UINT64_MAX) const;

        [[ nodiscard ]] inline vk::Semaphore get() const noexcept { return semaphore_; }
		[[ nodiscard ]] inline operator const vk::raii::Semaphore&() const noexcept { return semaphore_; }
		inline const vk::raii::Semaphore* operator->() const noexcept { return &semaphore_; }

    private:
        vk::raii::Semaphore semaphore_{VK_NULL_HANDLE};
    };

    class ConditionVariable
    {
    public:
        [[ nodiscard ]]explicit ConditionVariable() = default;
        [[ nodiscard ]]ConditionVariable(const vk::raii::Device& device);

        void notify_one() noexcept;
        void notify_all() noexcept;

        void wait( std::unique_lock<std::mutex>& lock );
        template< class Predicate >
        void wait( std::unique_lock<std::mutex>& lock, Predicate pred )
        {
            while (!pred())
                wait(lock);
        }
        [[ nodiscard ]] time_status wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time );
        template<class Predicate >
        bool wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time, Predicate pred )
        {
            while (!pred())
                if (wait_for(lock, rel_time) == time_status::timeout)
                    return pred();
            return true;
        }

        [[ nodiscard ]] inline vk::Semaphore get() const noexcept { return semaphore_; }
		[[ nodiscard ]] inline operator const vk::raii::Semaphore&() const noexcept { return semaphore_; }
		inline const vk::raii::Semaphore* operator->() const noexcept { return &semaphore_; }

    private:
        vk::raii::Semaphore semaphore_{VK_NULL_HANDLE};
        uint64_t signal_value_ = 0;
        
        void notify(uint64_t count);
    };

} // namespace vke