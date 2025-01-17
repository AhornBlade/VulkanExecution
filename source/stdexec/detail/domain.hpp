#pragma once

#include "sender.hpp"

namespace vke::exec
{
    struct default_domain 
    {
        template<sender Sndr, queryable... Env>
            requires (sizeof...(Env) <= 1)
        static constexpr sender decltype(auto) transform_sender(Sndr&& sndr, const Env&... env)
            noexcept(noexcept(tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...)))
            requires requires{ tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...); }
        {
            static_assert(requires{ {tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...)} -> sender; }, 
                "Customizations of transform_sender must return a sender.");
            return tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...);
        }

        template<sender Sndr, queryable... Env>
            requires (sizeof...(Env) <= 1)
        static constexpr sender decltype(auto) transform_sender(Sndr&& sndr, const Env&... env) noexcept
            requires (!requires{ tag_of_t<Sndr>().transform_sender(std::forward<Sndr>(sndr), env...); })
        {
            return std::forward<Sndr>(sndr);
        }

        template<sender Sndr, queryable Env>
        static constexpr queryable decltype(auto) transform_env(Sndr&& sndr, Env&& env) noexcept
        {
            if constexpr(requires{ tag_of_t<Sndr>().transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env)); })
            {
                static_assert(
                    requires{ {tag_of_t<Sndr>().transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env))} -> queryable; }, 
                    "Customizations of transform_env must return a queryable.");
                return tag_of_t<Sndr>().transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env));
            }
            else
            {
                return static_cast<Env>(env);
            }
        }

        template<class Tag, sender Sndr, class... Args>
        static constexpr decltype(auto) apply_sender(Tag, Sndr&& sndr, Args&&... args)
            noexcept( noexcept(Tag().apply_sender(std::forward<Sndr>(sndr), std::forward<Args>(args)...)) )
        {
            return Tag().apply_sender(std::forward<Sndr>(sndr), std::forward<Args>(args)...);
        }
    };

    namespace _domain
    {
        struct transform_sender_t
        {
            template<class Sndr>
            constexpr static sender auto operator()(auto _domain, Sndr&& sndr, const auto& ... envs)
            {
                sender auto&& transformed_sndr = [&]()
                {
                    if constexpr(requires{_domain.transform_sender(std::forward<Sndr>(sndr), envs...);})
                    {
                        static_assert(requires{{_domain.transform_sender(std::forward<Sndr>(sndr), envs...)} -> sender;}, 
                            "Customizations of transform_sender must return a sender.");
                        return _domain.transform_sender(std::forward<Sndr>(sndr), envs...);
                    }
                    else
                    {
                        return default_domain{}.transform_sender(std::forward<Sndr>(sndr), envs...);
                    }
                } ();

                if constexpr(std::same_as<std::remove_cvref_t<Sndr>, std::remove_cvref_t<decltype(transformed_sndr)>>)
                {
                    return std::forward<decltype(transformed_sndr)>(transformed_sndr);
                }
                else
                {
                    return transform_sender_t{}(_domain, std::forward<decltype(transformed_sndr)>(transformed_sndr), envs...);
                }
            }
        };

        struct transform_env_t
        {
            template<class Sndr, class Env>
            constexpr static queryable decltype(auto) transform_env(auto _domain, Sndr&& sndr, Env&& env) noexcept
            {
                if constexpr( requires{_domain.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env));} )
                {
                    static_assert(noexcept(_domain.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env))), 
                        "Customizations of transform_env must be noexcept.");
                    static_assert(requires{ {_domain.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env))} -> queryable; }, 
                        "Customizations of transform_env must return a queryable.");
                    return _domain.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env));
                }
                else
                {
                    return default_domain{}.transform_env(std::forward<Sndr>(sndr), std::forward<Env>(env));
                }
            }
        };

        struct apply_sender_t
        {
            template<class Tag, class Sndr, class ... Args>
            constexpr static decltype(auto) apply_sender(auto _domain, Tag, Sndr&& sndr, Args&&... args)
                noexcept(noexcept(_domain.apply_sender(Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...)))
                requires requires{_domain.apply_sender(Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);}
            {
                return _domain.apply_sender(Tag{}, std::forward<Sndr>(sndr), std::forward<Args>(args)...);
            }

            template<class Tag, class Sndr, class ... Args>
            constexpr static decltype(auto) apply_sender(auto _domain, Tag, Sndr&& sndr, Args&&... args)
                noexcept(noexcept(default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr))))
                requires requires{default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr));}
            {
                return default_domain().apply_sender(Tag(), std::forward<Sndr>(sndr));
            }
        };

    } // namespace _domain

    using _domain::transform_sender_t;
    using _domain::transform_env_t;
    using _domain::apply_sender_t;

    inline constexpr transform_sender_t transform_sender{};
    inline constexpr transform_env_t transform_env{};
    inline constexpr apply_sender_t apply_sender{};

    namespace _get_sender_domain
    {
        template<class Sndr, class Tag>
        using comple_domain_t = decltype(get_domain(get_completion_scheduler<Tag>(get_env(Sndr{}))));

        template<class Sndr>
        using common_comple_domain_t = std::common_type_t<
            comple_domain_t<Sndr, set_value_t>, 
            comple_domain_t<Sndr, set_error_t>,
            comple_domain_t<Sndr, set_stopped_t>>;

        template<class Sndr>
        concept has_comple_domain = 
            requires{ comple_domain_t<Sndr, set_value_t>{}; } ||
            requires{ comple_domain_t<Sndr, set_error_t>{}; } ||
            requires{ comple_domain_t<Sndr, set_stopped_t>{}; };

        template<class Default = default_domain, class Sndr>
        constexpr auto completion_domain(const Sndr& sndr) noexcept
            requires (!has_comple_domain<Sndr>) && std::is_default_constructible_v<Default>
        {
            return Default{};
        }

        template<class Default = default_domain, class Sndr>
        constexpr auto completion_domain(const Sndr& sndr) noexcept
            requires requires{common_comple_domain_t<Sndr>{};}
        {
            return common_comple_domain_t<Sndr>{};
        }

    } // namespace _get_sender_domain

    constexpr auto get_sender_domain(const auto& sndr, const auto& ... env) noexcept
    {
        static_assert(sizeof...(env) <= 1);
        if constexpr( requires{ get_domain(get_env(sndr)); } )
            return get_domain(get_env(sndr));
        else if constexpr(requires{ _get_sender_domain::completion_domain(sndr); } && sizeof...(env) == 0)
            return _get_sender_domain::completion_domain(sndr);
        else if constexpr(requires{ _get_sender_domain::completion_domain<void>(sndr); } && sizeof...(env) == 1)
            return _get_sender_domain::completion_domain<void>(sndr);
        else if constexpr(requires{ get_domain(env...); } && sizeof...(env) == 1)
            return get_domain(env...);
        else if constexpr(requires{ get_domain(get_scheduler(env...)); } && sizeof...(env) == 1)
            return get_domain(get_scheduler(env...));
        else
            return default_domain{};
    }

}// namespace vke::exec