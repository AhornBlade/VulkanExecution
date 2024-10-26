#pragma once

#include "utils.hpp"
#include <mutex>

namespace vke{

    class Semaphore
    {
    public:
        [[ nodiscard ]]explicit Semaphore() = default;
        [[ nodiscard ]]Semaphore(const vk::raii::Device& device, bool is_counting = false);

        void release(uint64_t update = 1);
        void acquire();

        [[ nodiscard ]] inline vk::Semaphore get() const noexcept { return semaphore_; }
		[[ nodiscard ]] inline operator const vk::raii::Semaphore&() const noexcept { return semaphore_; }
		inline const vk::raii::Semaphore* operator->() const noexcept { return &semaphore_; }

    private:
        vk::raii::Semaphore semaphore_{VK_NULL_HANDLE};
        bool is_counting_;
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
        std::cv_status wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time );
        template<class Predicate >
        bool wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time, Predicate pred )
        {
            while (!pred())
                if (wait_for(lock, rel_time) == std::cv_status::timeout)
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