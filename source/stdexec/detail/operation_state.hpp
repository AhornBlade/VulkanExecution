#pragma once

#include "concepts.hpp"

#include <functional>

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
        } &&
        std::movable<std::remove_cvref_t<O>>;

    namespace _operation_state_handle
    {
        struct operation_state_handle
        {
            using operation_state_concept = operation_state_t;

            operation_state_handle() = default;
            ~operation_state_handle() = default;
            operation_state_handle(operation_state_handle&& other) noexcept = default;
            operation_state_handle& operator=(operation_state_handle&& other) noexcept = default;
            operation_state_handle(const operation_state_handle&) = delete;
            operation_state_handle& operator=(const operation_state_handle&) = delete;

            template<class Op>
            operation_state_handle(Op&& op) 
                requires (!std::is_lvalue_reference_v<Op>) &&
                    (!std::same_as<std::remove_cvref_t<Op>, operation_state_handle>)
                : _func{ [op = std::forward<Op>(op)] mutable 
                { 
                    static_assert(std::move_constructible<Op>);
                    op.start(); 
                } } { }

            inline void start() & noexcept { if(_func) _func(); }
            std::move_only_function<void()> _func = nullptr;
        };
    } // namespace _operation_state_handle

    using _operation_state_handle::operation_state_handle;

} // namespace vke::exec