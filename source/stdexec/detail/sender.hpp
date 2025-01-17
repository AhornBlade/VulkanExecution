#pragma once

#include "env.hpp"
#include "completion_signatures.hpp"
#include "awaitable.hpp"
#include "operation_state.hpp"
#include "receiver.hpp"

namespace vke::exec
{
    namespace _sender
    {
        struct sender_t {};

        template<class S>
        concept enable_sender = 
            std::derived_from<typename S::sender_concept, sender_t> ||
            is_awaitable<S, env_promise<empty_env>>;
            
        template<class Sndr>
        concept sender =
            enable_sender<std::remove_cvref_t<Sndr>> &&
            requires (const std::remove_cvref_t<Sndr>& sndr) {
                { get_env(sndr) } -> queryable;
            } &&
            std::move_constructible<std::remove_cvref_t<Sndr>> &&
            std::constructible_from<std::remove_cvref_t<Sndr>, Sndr>;

        template<class Sndr, class Env = empty_env>
        concept sender_in =
            sender<Sndr> &&
            queryable<Env> &&
            requires (Sndr&& sndr, Env&& env) {
                { get_completion_signatures(std::forward<Sndr>(sndr), std::forward<Env>(env)) }
                -> valid_completion_signatures;
            };
    };

    using _sender::sender_t;
    using _sender::sender;
    using _sender::sender_in;

    namespace _connect
    {
        struct connect_t
        {
            template<sender Sndr, receiver Rcvr>
            constexpr static operation_state decltype(auto) operator()(Sndr&& sndr, Rcvr&& rcvr)
            {
                sender auto&& new_sndr = transform_sender(
                    decltype(get_sender_domain(std::forward<Sndr>(sndr), get_env(rcvr))){}, sndr, get_env(rcvr));

                using NewSndr = decltype(new_sndr);

                if constexpr(requires{ std::forward<NewSndr>(new_sndr).connect(std::forward<Rcvr>(rcvr)); })
                {
                    static_assert(requires{ {std::forward<NewSndr>(new_sndr).connect(std::forward<Rcvr>(rcvr))} -> operation_state; },
                        "Customizations of connect must return an operation_state.");
                    return std::forward<NewSndr>(new_sndr).connect(std::forward<Rcvr>(rcvr));
                }
                else if constexpr(requires{connect_awaitable(std::forward<NewSndr>, std::forward<Rcvr>(rcvr));})
                {
                    return connect_awaitable(std::forward<NewSndr>, std::forward<Rcvr>(rcvr));
                }
                else
                {
                    static_assert(false, "Failed to find Customizations of connect.");
                }
            }
        };

    } // namespace _connect

    using _connect::connect_t;

    inline constexpr connect_t connect{};

    template<sender Sndr, receiver Rcvr>
    using connect_result_t = std::invoke_result_t<connect_t, Sndr, Rcvr>;
    
} // namespace vke::exec