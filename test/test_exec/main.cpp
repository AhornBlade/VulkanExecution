#include "execution.hpp"

#include <iostream>

using namespace vke::exec;

int main()
{
    auto func1 = [](int c) -> int
    {
        return c;
    };

    auto func2 = [](auto e) noexcept -> int
    {
        return 1;
    };

    sender auto sndr1 = 
        just(1) | 
        then(func1) |
        upon_error(func2);


    // using Tag = completion_signatures_for<decltype(sndr0), empty_env>;
    using Tag1 = completion_signatures_for<decltype(sndr1), empty_env>;

    // auto sig = get_completion_signatures(sndr1, empty_env{});
}