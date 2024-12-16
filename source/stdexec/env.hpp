#pragma once

#include "concepts.hpp"
#include "stop_token.hpp"

namespace vke::exec
{
    namespace _queries
    {
        template<class Query, class QueryType>
        concept queryable_of = requires(Query&& q){ std::forward<Query>(q).query(QueryType{}); };

        template<class Query, class QueryType>
        concept nothrow_queryable_of = requires(Query&& q){ {std::forward<Query>(q).query(QueryType{})} noexcept; };

        template<class Query, class QueryType>
        using queryable_result_of_t = decltype(Query{}.query(QueryType{}));

        struct forwarding_query_t
        {
            using Tag = forwarding_query_t;

            static consteval bool operator()(auto _query) noexcept
            {
                if constexpr(queryable_of<decltype(_query), Tag>)
                {
                    static_assert(requires{std::bool_constant<_query.query(Tag{})>::value;}, 
                        "Customizations of forwarding_query must be consteval and return bool.");
                    return _query.query(Tag{});
                }
                else
                {
                    return std::derived_from<decltype(_query), Tag>;
                }
            }
        };

        struct get_allocator_t 
        {
            using Tag = get_allocator_t;

            static constexpr auto query(forwarding_query_t) noexcept -> bool {
                return true;
            }

            static auto operator()(const auto& _env) noexcept
                requires nothrow_queryable_of<decltype(_env), Tag>
            {
                return _env.query(Tag{});
            }
        };

        struct get_stop_token_t
        {
            using Tag = get_stop_token_t;

            static constexpr auto query(forwarding_query_t) noexcept -> bool 
            {
                return true;
            }

            static stoppable_token auto operator()(const auto& _env) noexcept
            {
                if constexpr(queryable_of<decltype(_env), Tag>)
                {
                    static_assert(nothrow_queryable_of<decltype(_env), Tag>,
                        "Customizations of get_stop_token must be noexcept.");
                    return _env.query(Tag{});
                }
                else
                {
                    return never_stop_token{};
                }
            }
        };

        struct get_domain_t 
        {
            using Tag = get_domain_t;

            static constexpr auto operator()(const auto& _env) noexcept
                requires nothrow_queryable_of<decltype(_env), Tag>
            {
                return _env.query(Tag{});
            }

            static constexpr auto query(forwarding_query_t) noexcept -> bool 
            {
                return true;
            }
        };

        struct get_scheduler_t 
        {
            using Tag = get_scheduler_t;

            static auto operator()(const auto& _env) noexcept
                requires nothrow_queryable_of<decltype(_env), Tag>
            {
                return _env.query(Tag{});
            }

            static constexpr auto query(forwarding_query_t) noexcept -> bool 
            {
                return true;
            }
        };
        
        struct get_delegation_scheduler_t 
        {
            using Tag = get_delegation_scheduler_t;

            static auto operator()(const auto& _env) noexcept
                requires nothrow_queryable_of<decltype(_env), Tag>
            {
                return _env.query(Tag{});
            }

            static constexpr auto query(forwarding_query_t) noexcept -> bool 
            {
                return true;
            }
        };

    } // namespace _queries

    using _queries::forwarding_query_t;
    using _queries::get_allocator_t;
    using _queries::get_stop_token_t;
    using _queries::get_domain_t;
    using _queries::get_scheduler_t;
    using _queries::get_delegation_scheduler_t;

    inline constexpr forwarding_query_t forwarding_query{};
    inline constexpr get_allocator_t get_allocator{};
    inline constexpr get_stop_token_t get_stop_token{};
    inline constexpr get_domain_t get_domain{};
    inline constexpr get_scheduler_t get_scheduler{};
    inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

    namespace _get_env
    {
        struct empty_env{};

        struct get_env_t
        {
            static queryable auto operator()(const auto& env_provider) noexcept
            {
                if constexpr(requires{ env_provider.get_env(); })
                {
                    static_assert(requires{ {env_provider.get_env()} noexcept; },
                        "Customizations of get_env must be noexcept.");
                    return env_provider.get_env();
                }
                else 
                {
                    return empty_env{};
                }
            }
        };

    } // namespace _get_env

    using _get_env::empty_env;
    using _get_env::get_env_t;

    inline constexpr get_env_t get_env{};

}// namespace vke::exec