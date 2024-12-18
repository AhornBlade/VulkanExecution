#pragma once

namespace vke::exec
{
    template<class C, class Promise>
    concept is_awaitable = true;

    template<class Env>
    struct env_promise {};

} // namespace vke::exec