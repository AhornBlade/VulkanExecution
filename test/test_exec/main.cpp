#include "execution.hpp"

#include <iostream>

using namespace vke::exec;

int main()
{
    auto func1 = [](char c) -> int
    {
        return c;
    };

    sender auto sndr1 = 
        just_error('1') |
        upon_error(func1);


    // using Tag = completion_signatures_for<decltype(sndr0), empty_env>;
    using Tag1 = completion_signatures_for<decltype(sndr1), empty_env>;

    // auto sig = get_completion_signatures(sndr1, empty_env{});
}