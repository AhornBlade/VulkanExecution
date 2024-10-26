#include "Synchronization.hpp"

#include <iostream>

namespace vke{

    const uint64_t counting_semaphore_init_value_ = 0x0000ffff;

    CountingSemaphore::CountingSemaphore(const vk::raii::Device& device, uint64_t desired)
    {
        vk::SemaphoreTypeCreateInfo typeCreateInfo{};
        typeCreateInfo.setSemaphoreType(vk::SemaphoreType::eTimeline);
        typeCreateInfo.setInitialValue(counting_semaphore_init_value_ + desired);

        vk::SemaphoreCreateInfo createInfo{};
        createInfo.setPNext(&typeCreateInfo);

        semaphore_ = device.createSemaphore(createInfo);
    }

    void CountingSemaphore::release(uint64_t update)
    {
        vk::SemaphoreSignalInfo signalInfo{};

        signalInfo.setSemaphore(semaphore_);
        signalInfo.setValue(semaphore_.getCounterValue() + update);
        
        semaphore_.getDispatcher()->vkSignalSemaphore(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreSignalInfo*>(&signalInfo));
    }

    time_status CountingSemaphore::acquire(uint64_t rel_time)
    {
        vk::SemaphoreSignalInfo signalInfo{};

        signalInfo.setSemaphore(semaphore_);
        signalInfo.setValue(semaphore_.getCounterValue() - 1);
        
        semaphore_.getDispatcher()->vkSignalSemaphore(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreSignalInfo*>(&signalInfo));

        vk::SemaphoreWaitInfo waitInfo{};

        waitInfo.setSemaphores(*semaphore_);
        waitInfo.setValues(counting_semaphore_init_value_);

        vk::Result r = static_cast<vk::Result>(semaphore_.getDispatcher()->vkWaitSemaphores(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreWaitInfo*>(&waitInfo), rel_time));

        if(r == vk::Result::eTimeout)
        {
            signalInfo.setValue(semaphore_.getCounterValue() + 1);
            
            semaphore_.getDispatcher()->vkSignalSemaphore(semaphore_.getDevice(), 
                reinterpret_cast<VkSemaphoreSignalInfo*>(&signalInfo));

            return time_status::timeout;
        }
        else
        {
            return time_status::no_timeout;
        }
    }

    BinarySemaphore::BinarySemaphore(const vk::raii::Device& device)
    {
        vk::SemaphoreCreateInfo createInfo{};

        semaphore_ = device.createSemaphore(createInfo);
    }

    time_status BinarySemaphore::acquire(uint64_t rel_time) const
    {
        vk::SemaphoreWaitInfo waitInfo{};

        waitInfo.setSemaphores(*semaphore_);

        return static_cast<vk::Result>(semaphore_.getDispatcher()->vkWaitSemaphores(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreWaitInfo*>(&waitInfo), rel_time)) == vk::Result::eTimeout ? 
            time_status::timeout : time_status::no_timeout;
    }

    ConditionVariable::ConditionVariable(const vk::raii::Device& device)
    {
        vk::SemaphoreTypeCreateInfo typeCreateInfo{};
        typeCreateInfo.setSemaphoreType(vk::SemaphoreType::eTimeline);
        typeCreateInfo.setInitialValue(0);

        vk::SemaphoreCreateInfo createInfo{};
        createInfo.setPNext(&typeCreateInfo);

        semaphore_ = device.createSemaphore(createInfo);
    }

    void ConditionVariable::notify(uint64_t count)
    {
        vk::SemaphoreSignalInfo signalInfo{};

        signalInfo.setSemaphore(semaphore_);
        signalInfo.setValue(count);
        
        semaphore_.getDispatcher()->vkSignalSemaphore(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreSignalInfo*>(&signalInfo));
    }

    void ConditionVariable::notify_one() noexcept
    {
        notify(semaphore_.getCounterValue() + 1);
    }

    void ConditionVariable::notify_all() noexcept
    {
        notify(signal_value_);
    }

    void ConditionVariable::wait( std::unique_lock<std::mutex>& lock )
    {
        time_status ts = wait_for(lock, UINT64_MAX);

        if(ts == time_status::timeout)
        {
            std::cout << "Condition variable time out\n";
        }
    }

    time_status ConditionVariable::wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time )
    {
        vk::SemaphoreWaitInfo waitInfo{};

        signal_value_++;

        waitInfo.setSemaphores(*semaphore_);
        waitInfo.setValues(signal_value_);

        lock.unlock();

        vk::Result r = static_cast<vk::Result>(semaphore_.getDispatcher()->vkWaitSemaphores(semaphore_.getDevice(), 
            reinterpret_cast<VkSemaphoreWaitInfo*>(&waitInfo), rel_time));

        lock.lock();

        return r == vk::Result::eTimeout ? time_status::timeout : time_status::no_timeout;
    }

} // namespace vke