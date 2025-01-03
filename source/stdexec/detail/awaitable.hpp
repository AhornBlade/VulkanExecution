#pragma once

namespace vke::exec
{
    template<class C, class Promise>
    concept is_awaitable = false;

    template<class Env>
    struct env_promise {};

    template<class Sndr, class Env>
    using await_result_type = int; // to do

    auto connect_awaitable(auto&& sndr, auto&& rcvr);

} // namespace vke::exec