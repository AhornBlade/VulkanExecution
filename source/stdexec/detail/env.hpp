#pragma once

#include "concepts.hpp"
#include "stop_token.hpp"

#include <tuple>

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

    template <class _Tag>
    concept completion_tag = one_of<_Tag, set_value_t, set_error_t, set_stopped_t>;

    enum class forward_progress_guarantee 
    {
        concurrent,
        parallel,
        weakly_parallel
    };

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

            static decltype(auto) operator()(const auto& _env) noexcept
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

            static stoppable_token decltype(auto) operator()(const auto& _env) noexcept
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

            static constexpr decltype(auto) operator()(const auto& _env) noexcept
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

            static decltype(auto) operator()(const auto& _env) noexcept
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

            static decltype(auto) operator()(const auto& _env) noexcept
                requires nothrow_queryable_of<decltype(_env), Tag>
            {
                return _env.query(Tag{});
            }

            static constexpr auto query(forwarding_query_t) noexcept -> bool 
            {
                return true;
            }
        };

        struct get_forward_progress_guarantee_t
        {
            using Tag = get_forward_progress_guarantee_t;

            static constexpr auto operator()(auto&& _t) 
                noexcept(nothrow_queryable_of<decltype(_t), Tag>)
                -> forward_progress_guarantee
                requires queryable_of<decltype(_t), Tag>
            {
                static_assert(std::convertible_to<queryable_result_of_t<decltype(_t), Tag>, forward_progress_guarantee>,
                    "Customizations of get_forward_progress_guarantee must return forward_progress_guarantee.");
                return _t.query(Tag{});
            }

            static constexpr auto operator()(auto&&) noexcept -> forward_progress_guarantee
            {
                return forward_progress_guarantee::weakly_parallel;
            } 
        };

        template<completion_tag T>
        struct get_completion_scheduler_t 
        {
            using Tag = get_completion_scheduler_t;

            static decltype(auto) operator()(const auto& q) noexcept
                requires nothrow_queryable_of<decltype(q), Tag>
            {
                return q.query(Tag{});
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
    using _queries::get_forward_progress_guarantee_t;
    using _queries::get_completion_scheduler_t;

    inline constexpr forwarding_query_t forwarding_query{};
    inline constexpr get_allocator_t get_allocator{};
    inline constexpr get_stop_token_t get_stop_token{};
    inline constexpr get_domain_t get_domain{};
    inline constexpr get_scheduler_t get_scheduler{};
    inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};
    inline constexpr get_forward_progress_guarantee_t get_forward_progress_guarantee{};
    template<completion_tag T>
    inline constexpr get_completion_scheduler_t<T> get_completion_scheduler{};

    namespace _env
    {
        template<class ... Envs>
        struct env : decayed_tuple<Envs...>
        {
            using decayed_tuple<Envs...>::decayed_tuple;
        };

        template<>
        struct env<> {};

        using empty_env = env<>;

    } // namespace _env

    using _env::env;
    using _env::empty_env;

    namespace _get_env
    {
        struct get_env_t
        {
            static queryable decltype(auto) operator()(const auto& env_provider) noexcept
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

    using _get_env::get_env_t;

    inline constexpr get_env_t get_env{};

    template <class EnvProvider>
    using env_of_t = std::invoke_result_t<get_env_t, EnvProvider>;

}// namespace vke::exec