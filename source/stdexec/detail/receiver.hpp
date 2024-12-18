#pragma once

#include "env.hpp"
#include "completion_signatures.hpp"

namespace vke::exec
{
    namespace _rcvrs 
    {
        struct set_value_t
        {
            template<class Rcvr, class ... Args>
            static void operator()(Rcvr&& receiver, Args&& ... args) noexcept
                requires requires{ std::forward<Rcvr>(receiver).set_value(std::forward<Args>(args)...); }
            {        
                static_assert(noexcept(std::forward<Rcvr>(receiver).set_value(std::forward<Args>(args)...)),
                    "set_value member functions must be noexcept");
                static_assert(std::same_as<void, 
                    decltype(std::forward<Rcvr>(receiver).set_value(std::forward<Args>(args)...))>,
                    "set_value member functions must return void");
                std::forward<Rcvr>(receiver).set_value(std::forward<Args>(args)...);
            } 
        };

        struct set_error_t
        {
            template<class Rcvr, class Err>
            static void operator()(Rcvr&& receiver, Err&& error) noexcept
                requires requires{ std::forward<Rcvr>(receiver).set_error(std::forward<Err>(error)); }
            {        
                static_assert(noexcept(std::forward<Rcvr>(receiver).set_error(std::forward<Err>(error))),
                    "set_error member functions must be noexcept");
                static_assert(std::same_as<void, 
                    decltype(std::forward<Rcvr>(receiver).set_error(std::forward<Err>(error)))>,
                    "set_error member functions must return void");
                std::forward<Rcvr>(receiver).set_error(std::forward<Err>(error));
            } 
        };

        struct set_stopped_t
        {
            template<class Rcvr>
            static void operator()(Rcvr&& receiver) noexcept
                requires requires { std::forward<Rcvr>(receiver).set_stopped(); }
            {        
                static_assert(noexcept(std::forward<Rcvr>(receiver).set_stopped()),
                    "set_stopped member functions must be noexcept");
                static_assert(std::same_as<void, decltype(std::forward<Rcvr>(receiver).set_stopped())>,
                    "set_stopped member functions must return void");
                std::forward<Rcvr>(receiver).set_stopped();
            } 
        };
    } // namespace _rcvrs

    using _rcvrs::set_value_t;
    using _rcvrs::set_error_t;
    using _rcvrs::set_stopped_t;

    inline constexpr set_value_t set_value{};
    inline constexpr set_error_t set_error{};
    inline constexpr set_stopped_t set_stopped{};

    struct receiver_t {};

    template<class Rcvr>
    concept receiver =
        std::derived_from<typename std::remove_cvref_t<Rcvr>::receiver_concept, receiver_t> &&
        requires(const std::remove_cvref_t<Rcvr>& rcvr) 
        {
            { get_env(rcvr) } -> queryable;
        } &&
        std::move_constructible<std::remove_cvref_t<Rcvr>> &&
        std::constructible_from<std::remove_cvref_t<Rcvr>, Rcvr>;

    namespace _rcvrs
    {
        template<class Signature, class Rcvr>
        concept valid_completion_for =
        requires (Signature* sig) {
            []<class Tag, class... Args>(Tag(*)(Args...))
                requires std::invocable<Tag, std::remove_cvref_t<Rcvr>, Args...>
            {}(sig);
        };

        template<class Rcvr, class Completions>
        concept has_completions =
        requires (Completions* completions) {
            []<valid_completion_for<Rcvr>...Sigs>(completion_signatures<Sigs...>*)
            {}(completions);
        };

        template<class Rcvr, class Completions>
        concept receiver_of = receiver<Rcvr> && has_completions<Rcvr, Completions>;
    }

    using _rcvrs::receiver_of;

}// namespace vke::exec