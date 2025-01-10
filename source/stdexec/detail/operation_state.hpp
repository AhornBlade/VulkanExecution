#pragma once

#include <memory>

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

    namespace _operation_state_handle
    {
        struct operation_state_base
        {
            operation_state_base() = default;
            virtual ~operation_state_base() noexcept = default;

            virtual void start() & noexcept = 0;
        };

        template<class Op>
        struct operation_state_impl : operation_state_base
        {
            operation_state_impl(Op&& op) : _op{std::move(op)} {}

            virtual void start() & noexcept
            {
                _op.start();
            }

            Op _op;
        };

        struct operation_state_handle
        {
            operation_state_handle() = default;

            template<operation_state Op>
            operation_state_handle(Op&& op) : _handle{ new operation_state_impl<std::decay_t<Op>>(std::move(op)) } {}
            
            template<operation_state Op>
            operation_state_handle(Op& op) = delete;

            operation_state_handle(operation_state_handle&&) noexcept = default;
            operation_state_handle& operator=(operation_state_handle&&) noexcept = default;

            void start() & noexcept
            {
                _handle->start();
            }

            std::unique_ptr<operation_state_base> _handle;
        };
    } // namespace _operation_state_handle

    using _operation_state_handle::operation_state_handle;

} // namespace vke::exec