#include <execution.hpp>

#include <iostream>

using namespace vke::exec;

struct test_receiver 
{
    using receiver_concept = receiver_t;

    template<class... Args>
    void set_value(Args&&... args) && noexcept 
    {
        std::cout << "set_value" << (... << args) << '\n';
    }

    template<class Error>
    void set_error(Error&& err) && noexcept 
    {
        try 
        {
            std::rethrow_exception(err);
        } catch (const std::exception& e) {
            std::cout << "set_error" << e.what();
        }
    }

    void set_stopped() && noexcept
    {
        std::cout << "set_stopped" << '\n';
    }

    auto get_env() const noexcept
    {
        return empty_env{};
    }
};


int main()
{
    auto func1 = []()
    {
        return std::this_thread::get_id();
    };

    auto func2 = [](auto e) noexcept -> int
    {
        return 1;
    };

    thread_pool pool1{1};
    thread_pool pool2{1};

    std::cout << "main thread " << std::this_thread::get_id() << '\n';

    sender auto sndr1 = 
        schedule(pool1.get_scheduler()) |
        then(func1);

    auto op = connect(sndr1, test_receiver{});

    op.start();

    // using Tag = completion_signatures_for<decltype(sndr0), empty_env>;
    using Tag1 = completion_signatures_for<decltype(sndr1), empty_env>;

    // auto sig = get_completion_signatures(sndr1, empty_env{});
}