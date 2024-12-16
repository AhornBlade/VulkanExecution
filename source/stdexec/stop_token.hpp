#pragma once

#include <stop_token>

#include "concepts.hpp"

namespace vke::exec
{
    struct never_stop_token 
    {
    private:
        struct __callback_type 
        {
            explicit __callback_type(never_stop_token, auto&&) noexcept {}
        };
    public:
        template <class>
        using callback_type = __callback_type;

        static constexpr auto stop_requested() noexcept -> bool 
        {
            return false;
        }

        static constexpr auto stop_possible() noexcept -> bool 
        {
            return false;
        }

        auto operator==(const never_stop_token&) const noexcept -> bool = default;
    };

  template <class Token>
  concept stoppable_token =
    std::is_nothrow_copy_constructible_v<Token> && 
    std::is_nothrow_move_constructible_v<Token> && 
    std::equality_comparable<Token> && 
    requires(const Token& token) 
    {
        { token.stop_requested() } noexcept -> std::convertible_to<bool>;
        { token.stop_possible() } noexcept -> std::convertible_to<bool>;
    };

}// namespace vke::exec