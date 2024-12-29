#include "execution.hpp"

#include <iostream>

using namespace vke::exec;

int main()
{
    auto func1 = [](auto&& ... args) { (std::cout << ... << args); };

    sender auto sndr1 = then(func1, just(1, 2, 3));
    sender auto sndr2 = 
        just(1, 2, 3) |
        then(func1);
}