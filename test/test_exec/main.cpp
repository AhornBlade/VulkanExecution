#include "execution.hpp"

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
    auto func1 = [](int c) -> int
    {
        throw std::runtime_error("test runtime error");
        return c;
    };

    auto func2 = [](auto e) noexcept -> int
    {
        return 1;
    };

    sender auto sndr1 = 
        just(1) | 
        then(func1);

    auto op = connect(sndr1, test_receiver{});

    op.start();

    // using Tag = completion_signatures_for<decltype(sndr0), empty_env>;
    using Tag1 = completion_signatures_for<decltype(sndr1), empty_env>;

    // auto sig = get_completion_signatures(sndr1, empty_env{});
}