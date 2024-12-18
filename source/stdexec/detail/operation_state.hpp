#pragma once

#include "concepts.hpp"

namespace vke::exec
{
    namespace _start
    {
        struct start_t
        {
            static void operator()(auto& op) noexcept
                requires requires{ op.start(); }
            {
                static_assert(noexcept(op.start()), "start() members must be noexcept");
                static_assert(std::same_as<void, decltype(op.start())>, "start() members must return void");
                op.start();
            }
        };
    };

    using _start::start_t;

    inline constexpr start_t start{};

    struct operation_state_t {};

    template<class O>
    concept operation_state =
        std::derived_from<typename O::operation_state_concept, operation_state_t> &&
        std::is_object_v<O> &&
        requires (O& o) 
        {
            { start(o) } noexcept;
        };

} // namespace vke::exec