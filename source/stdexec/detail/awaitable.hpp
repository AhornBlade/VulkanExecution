#pragma once

namespace vke::exec
{
    template<class C, class Promise>
    concept is_awaitable = true;

    template<class Env>
    struct env_promise {};

    template<class Sndr, class Env>
    using await_result_type = int; // to do

} // namespace vke::exec