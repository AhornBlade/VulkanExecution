#include "Synchronization.hpp"

#include <iostream>

namespace vke{

    const uint64_t counting_semaphore_init_value_ = 0x0000ffff;

    Semaphore::Semaphore(const vk::raii::Device& device, bool is_counting)
    {
        is_counting_ = is_counting;

        vk::SemaphoreTypeCreateInfo typeCreateInfo{};
        if(is_counting)
        {
            typeCreateInfo.setSemaphoreType(vk::SemaphoreType::eTimeline);
            typeCreateInfo.setInitialValue(counting_semaphore_init_value_);
        }
        else
        {
            typeCreateInfo.setSemaphoreType(vk::SemaphoreType::eBinary);
            typeCreateInfo.setInitialValue(0);
        }

        vk::SemaphoreCreateInfo createInfo{};
        createInfo.setPNext(&typeCreateInfo);

        semaphore_ = device.createSemaphore(createInfo);
    }

    void Semaphore::release(uint64_t update)
    {
        vk::SemaphoreSignalInfo signalInfo{};

        signalInfo.setSemaphore(semaphore_);
        if(is_counting_)
        {
            signalInfo.setValue(semaphore_.getCounterValue() + update);
        }
        else
        {
            signalInfo.setValue(1);
        }
        
        semaphore_.getDevice().signalSemaphore(signalInfo);
    }

    void Semaphore::acquire()
    {
        vk::SemaphoreSignalInfo signalInfo{};

        signalInfo.setSemaphore(semaphore_);
        signalInfo.setValue(semaphore_.getCounterValue() - 1);
        
        semaphore_.getDevice().signalSemaphore(signalInfo);

        vk::SemaphoreWaitInfo waitInfo{};

        uint64_t value = is_counting_ ? counting_semaphore_init_value_ : 1;

        waitInfo.setSemaphores(*semaphore_);
        waitInfo.setValues(value);

        if(semaphore_.getDevice().waitSemaphores(waitInfo, UINT64_MAX) == vk::Result::eTimeout)
        {
            std::cout << "Semaphore wait time out\n";
        }
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
        
        semaphore_.getDevice().signalSemaphore(signalInfo);
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
        wait_for(lock, UINT64_MAX);
    }

    std::cv_status ConditionVariable::wait_for( std::unique_lock<std::mutex>& lock, uint64_t rel_time )
    {
        vk::SemaphoreWaitInfo waitInfo{};

        signal_value_++;

        waitInfo.setSemaphores(*semaphore_);
        waitInfo.setValues(signal_value_);

        lock.unlock();

        vk::Result r = semaphore_.getDevice().waitSemaphores(waitInfo, rel_time);

        lock.lock();

        return r == vk::Result::eTimeout ? std::cv_status::timeout : std::cv_status::no_timeout;
    }

} // namespace vke