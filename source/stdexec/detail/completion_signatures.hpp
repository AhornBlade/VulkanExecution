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
        struct DefaultSetFunc
        {
            template<class ... Args>
            using DefaultSetValue = set_value_t(Args...);

            template<class Error>
            using DefaultSetError = set_error_t(Error);

            using DefaultSetStopped = set_stopped_t();
        };

        template<class, class>
        struct transform_sig_helper;

        template<class SetFunc, class ... Args>
        struct transform_sig_helper<SetFunc, set_value_t(Args...)>
        {
            using Type = SetFunc::template SetValue<Args...>;
        };

        template<class SetFunc, class Error>
        struct transform_sig_helper<SetFunc, set_error_t(Error)>
        {
            using Type = SetFunc::template SetError<Error>;
        };

        template<class SetFunc>
        struct transform_sig_helper<SetFunc, set_stopped_t()>
        {
            using Type = SetFunc::SetStopped;
        };

        template<class Sig, class SetFunc>
        using transform_sig_t = transform_sig_helper<SetFunc, Sig>::Type;

        template<class Sigs, class SetFunc, template<class ... > typename Variant, class More>
        struct transform_sigs_helper2;

        template<class ... Sig, class SetFunc, template<class ... > typename Variant, class ... More>
        struct transform_sigs_helper2<
            completion_signatures<Sig...>, SetFunc, Variant, Variant<More...>>
        {
            using Type = Variant<transform_sig_t<Sig, SetFunc>..., More...>;
        };

    } // namespace _signatures

    using _signatures::DefaultSetFunc;

    template<class Sigs, class SetFunc = DefaultSetFunc,
        template<class ... > typename Variant = completion_signatures,
        class More = Variant<>
        >
    using transform_completion_signatures = 
        _signatures::transform_sigs_helper2<Sigs, SetFunc, Variant, More>::Type;

    namespace _domain
    {
        struct transform_sender_t;
    }

    using _domain::transform_sender_t;
    
    extern const transform_sender_t transform_sender;

    constexpr auto get_sender_domain(const auto& sndr, const auto& ... env) noexcept;

    namespace _signatures
    {
        struct get_completion_signatures_t
        {
            static constexpr auto operator()(auto&& sndr, auto&& env) noexcept
                requires requires {transform_sender(decltype(get_sender_domain(sndr, env)){}, sndr, env);}
            {
                auto new_sndr = transform_sender(decltype(get_sender_domain(sndr, env)){}, sndr, env);

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
    using completion_signatures_for = decltype(get_completion_signatures(std::declval<Sndr>(), std::declval<Env>()));

} // namespace vke::exec