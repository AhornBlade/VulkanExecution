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

            template<movable_value ... Args>
            constexpr static sender decltype(auto) operator()(Args&& ... args)
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

    namespace _read_env
    {
        struct read_env_t
        {
            using Tag = read_env_t;

            template<class Q>
            constexpr static sender decltype(auto) operator()(Q&& q)
            {
                return _basic::basic_sender{Tag{}, std::forward<Q>(q)};
            }
        };
    }

    using _read_env::read_env_t;

    inline constexpr read_env_t read_env{};

    namespace _basic
    {
        template<>
        struct impls_for<_read_env::read_env_t> : default_impls {
            static constexpr auto start =
            [](auto query, auto& rcvr) noexcept -> void 
            {
                try
                {
                    if constexpr(std::is_void_v<decltype(query(get_env(rcvr)))>)
                    {
                        query(get_env(rcvr));
                        set_value(std::move(rcvr));
                    }
                    else
                    {
                        set_value(std::move(rcvr), query(get_env(rcvr)));
                    }
                } 
                catch (...) 
                {
                    set_error(std::move(rcvr), std::current_exception());
                }
            };
        };
    }

    namespace _upon
    {
        template<completion_tag CPO>
        struct upon_tag_t
        {
            using Tag = upon_tag_t;

            template<sender Sndr, movable_value Func>
            constexpr static sender decltype(auto) operator()(Sndr&& sndr, Func&& func)
            {
                return _basic::basic_sender{Tag{}, std::forward<Func>(func), std::forward<Sndr>(sndr)};
            }

            template<movable_value Func>
            constexpr static decltype(auto) operator()(Func&& func)
            {
                return sender_adaptor_closure{Tag{}, std::forward<Func>(func)};
            }
        };
    } // namespace _upon

    using then_t = _upon::upon_tag_t<set_value_t>;
    using upon_error_t = _upon::upon_tag_t<set_error_t>;
    using upon_stopped_t = _upon::upon_tag_t<set_stopped_t>;

    inline constexpr then_t then{};
    inline constexpr upon_error_t upon_error{};
    inline constexpr upon_stopped_t upon_stopped{};

    namespace _basic
    {
        template<completion_tag CPO>
        struct impls_for<_upon::upon_tag_t<CPO>> : default_impls {
            static constexpr auto complete = 
            []<class Tag, class... Args>
            (auto, auto& fn, auto& rcvr, Tag, Args&&... args) noexcept -> void 
            {
                if constexpr (std::same_as<Tag, CPO>) 
                {
                    try {
                        if constexpr(std::is_void_v<decltype(std::invoke(std::move(fn), std::forward<Args>(args)...))>)
                        {
                            std::invoke(std::move(fn), std::forward<Args>(args)...);
                            set_value(std::move(rcvr));
                        }
                        else
                        {
                            set_value(std::move(rcvr), std::invoke(std::move(fn), std::forward<Args>(args)...));
                        }
                    } catch (...) {
                        set_error(std::move(rcvr), std::current_exception());
                    }

                } else {
                    Tag()(std::move(rcvr), std::forward<Args>(args)...);
                }
            };
        };
    }

    

}// namespace vke::exec