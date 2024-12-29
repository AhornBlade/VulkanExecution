#pragma once

#include "basic.hpp"

namespace vke::exec
{
    namespace _just
    {
        template<completion_tag CPO>
        struct just_tag_t
        {
            using Tag = just_tag_t;

            template<class ... Args>
            constexpr static sender auto operator()(Args&& ... args)
            {
                static_assert(!(std::same_as<CPO, set_error_t> && sizeof...(args) != 1), 
                    "set_error can only accept 1 argument");
                static_assert(!(std::same_as<CPO, set_stopped_t> && sizeof...(args) != 0), 
                    "set_stopped cannot accept any arguments");
                return _basic::basic_sender{Tag{}, base_tuple{std::forward<Args>(args)...} };
            }
        };
        
    }
    
    using just_t = _just::just_tag_t<set_value_t>;
    using just_error_t = _just::just_tag_t<set_error_t>;
    using just_stopped_t = _just::just_tag_t<set_stopped_t>;

    inline constexpr just_t just{};
    inline constexpr just_error_t just_error{};
    inline constexpr just_stopped_t just_stopped{};

    namespace _basic
    {
        template<completion_tag CPO>
        struct impls_for<_just::just_tag_t<CPO>> : default_impls {
            static constexpr auto start =
            [](auto& state, auto& rcvr) noexcept -> void {
                state.apply([&](auto& ... args){
                    CPO{}(std::move(rcvr), std::move(args)...);
                });
            };
        };
    }

}// namespace vke::exec