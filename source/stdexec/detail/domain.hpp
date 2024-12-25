#pragma once

#include "sender.hpp"

namespace vke::exec
{
    struct transform_sender_t
    {
        constexpr static sender auto operator()(auto _domain, auto&& sndr, const auto& ... envs);
    };

    inline constexpr transform_sender_t transform_sender{};

}// namespace vke::exec