#pragma once

namespace vke::exec
{
    template<class C, class Promise>
    concept is_awaitable = false;

    template<class Env>
    struct env_promise {};

    template<class Sndr, class Env>
    using await_result_type = int; // to do

    template<class T>
    concept always_false = false;

    auto connect_awaitable(auto&& sndr, auto&& rcvr)
        requires always_false<decltype(sndr)>
    {
        return 1;
    }

} // namespace vke::exec