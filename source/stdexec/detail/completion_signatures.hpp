#pragma once

#include "concepts.hpp"
#include "awaitable.hpp"

#include <exception>

namespace vke::exec
{
    namespace _rcvrs 
    {
        struct set_value_t;
        struct set_error_t;
        struct set_stopped_t;
    } // namespace _rcvrs

    using _rcvrs::set_value_t;
    using _rcvrs::set_error_t;
    using _rcvrs::set_stopped_t;
    extern const set_value_t set_value;
    extern const set_error_t set_error;
    extern const set_stopped_t set_stopped;

    namespace _signatures
    {
        template<class Sig>
        struct is_compl_sig : std::false_type {};

        template<class ... Args>
        struct is_compl_sig<set_value_t(Args...)> : std::true_type {};

        template<class Error>
        struct is_compl_sig<set_error_t(Error)> : std::true_type {};

        template<>
        struct is_compl_sig<set_stopped_t()> : std::true_type {};

        template<class T>
        inline constexpr bool is_compl_sig_v = is_compl_sig<T>::value;

        template<class ...>
        struct completion_signatures{};

        template<class T>
        struct is_completion_signatures : std::false_type {};

        template<class ... Ts>
        struct is_completion_signatures<completion_signatures<Ts...>> : std::true_type {};

        template<class T>
        inline constexpr bool is_completion_signatures_v = is_completion_signatures<T>::value;
    };

    using _signatures::completion_signatures;

    template<class Signs>
    concept valid_completion_signatures = _signatures::is_completion_signatures_v<Signs>;

    namespace _signatures
    {
        template<template<class ...> typename SetValue, template<class> typename, class, class ... Values>
        auto transform_sig_helper(set_value_t(Values...)) -> SetValue<Values...>;

        template<template<class ...> typename, template<class> typename SetError, class, class Error>
        auto transform_sig_helper(set_error_t(Error)) -> SetError<Error>;
        
        template<template<class ...> typename, template<class> typename, class SetStopped>
        auto transform_sig_helper(set_stopped_t()) -> SetStopped;

        template<
            class Sig,
            template<class ...> typename SetValue, 
            template<class> typename SetError, 
            class SetStopped>
        using transform_sig_t = decltype(transform_sig_helper<SetValue, SetError, SetStopped>(Sig{}));

        template<
            template<class ...> typename SetValue, 
            template<class> typename SetError, 
            class SetStopped,
            template<class ... > typename Variant,
            class ... More,
            class ... Sigs
            >
        auto transform_sigs_fn(completion_signatures<Sigs...> *) 
            -> Variant<transform_sig_t<Sigs, SetValue, SetError, SetStopped>...>;

        template<
            class Sigs,
            template<class ...> typename SetValue, 
            template<class> typename SetError, 
            class SetStopped,
            template<class ... > typename Variant,
            class ... More
            >
        using transform_completion_signatures = 
            decltype(transform_sigs_fn<SetValue, SetError, SetStopped, Variant, More...>(Sigs{}));

    } // namespace _signatures

    struct transform_sender_t;
    
    extern const transform_sender_t transform_sender;

    namespace _signatures
    {
        struct get_completion_signatures_t
        {
            static constexpr auto operator()(auto&& sndr, auto&& env) noexcept
                requires requires {sndr.get_domain(env) -> sender;}
            {
                auto new_sndr = sndr.get_domain(env);

                if constexpr(requires{new_sndr.get_completion_signatures(env);})
                {
                    return decltype(new_sndr.get_completion_signatures(env)){};
                }
                else if constexpr(requires{typename std::remove_cvref_t<decltype(new_sndr)>::completion_signatures;})
                {
                    return typename std::remove_cvref_t<decltype(new_sndr)>::completion_signatures{};
                }
                else if constexpr(is_awaitable<decltype(new_sndr), env_promise<decltype(env)>>)
                {
                    return completion_signatures<
                        set_value_t(await_result_type<decltype(new_sndr),
                            env_promise<decltype(env)>>),
                        set_error_t(std::exception_ptr),
                        set_stopped_t()> {};
                }
                else
                {
                    static_assert(false, "Failed to find Customizations of get_completion_signatures");
                }
            }
        };

    } // namespace _signatures

    using _signatures::get_completion_signatures_t;

    inline constexpr get_completion_signatures_t get_completion_signatures{};

    template<class Sndr, class Env>
    using completion_signatures_for = decltype(get_completion_signatures(Sndr{}, Env{}));

} // namespace vke::exec