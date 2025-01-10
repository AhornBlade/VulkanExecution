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

struct CompleSigValue : DefaultSetFunc
{
    template<class ... Args>
    using SetValue = decayed_tuple<Args...>;
};

int main()
{
    auto func1 = [](int i) noexcept
    {
        return 42;
    };

    auto func2 = [](auto&& e)
    {
        return just(e + 42);
    };

    thread_pool pool1{1};
    thread_pool pool2{1};

    std::cout << "main thread " << std::this_thread::get_id() << '\n';

    sender auto sndr1 = 
        // schedule(pool1.get_scheduler()) |
        just(1) | 
        then(func1) |
        let(func2);

    auto op = connect(sndr1, test_receiver{});

    start(op);

    // using Tag = completion_signatures_for<decltype(sndr0), empty_env>;
    using Tag1 = completion_signatures_for<decltype(sndr1), empty_env>;

    // auto sig = get_completion_signatures(sndr1, empty_env{});
}