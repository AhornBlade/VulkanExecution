#pragma once

#include "env.hpp"
#include "sender.hpp"

namespace vke::exec
{
    namespace _schedule
    {
        struct schedule_t
        {
            template<class Sch>
            static sender decltype(auto) operator()(Sch&& sch) 
                noexcept(noexcept(std::forward<Sch>(sch).schedule()))
                requires requires{ { std::forward<Sch>(sch).schedule() } -> sender;}
            {
                return std::forward<Sch>(sch).schedule();
            }
        };

    }// namespace _schedule

    using _schedule::schedule_t;

    inline constexpr schedule_t schedule{};

    struct scheduler_t {};

    template<class Sch>
    concept scheduler =
        std::derived_from<typename std::remove_cvref_t<Sch>::scheduler_concept, scheduler_t> &&
        queryable<Sch> &&
        requires(Sch&& sch) {
            { schedule(std::forward<Sch>(sch)) } -> sender;
            { auto(get_completion_scheduler<set_value_t>(
                get_env(schedule(std::forward<Sch>(sch))))) }
                -> std::same_as<std::remove_cvref_t<Sch>>;
        } &&
        std::equality_comparable<std::remove_cvref_t<Sch>> &&
        std::copy_constructible<std::remove_cvref_t<Sch>>;
    
} // namespace vke::exec