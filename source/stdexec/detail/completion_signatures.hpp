#pragma once

#include "concepts.hpp"

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

} // namespace vke::exec