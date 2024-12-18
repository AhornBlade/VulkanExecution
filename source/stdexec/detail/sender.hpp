#pragma once

#include "env.hpp"
#include "completion_signatures.hpp"
#include "awaitable.hpp"

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

    using _sender::sender;
    using _sender::sender_in;
    
} // namespace vke::exec