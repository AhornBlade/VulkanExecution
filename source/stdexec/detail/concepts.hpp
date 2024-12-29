#pragma once

#include <concepts>
#include <tuple>

namespace vke::exec
{
    template<class T>
    concept movable_value = 
        std::move_constructible<std::decay_t<T>> && 
        std::constructible_from<std::decay_t<T>, T> &&
        (!std::is_array_v<std::remove_reference_t<T>>);

    template<class From, class To>
    concept decays_to = std::same_as<std::decay_t<From>, To>;

    template<class T>
    concept class_type = decays_to<T, T> && std::is_class_v<T>;

    template<class T>
    concept queryable = std::destructible<T>;
    
    template <class _Ty, class... _As>
    concept one_of = (std::same_as<_Ty, _As> || ...);

    namespace _mconcat
    {
        template<class ... TypeLists>
        struct mconcat_impl;

        template<template<class ...> typename ListType, class ... Ts, class ... Us>
        struct mconcat_impl<ListType<Ts...>, ListType<Us...>>
        {
            using Type = ListType<Ts..., Us...>;
        };

        template<template<class ...> typename ListType, class ... Ts, class ... Us, class ... TypeLists>
        struct mconcat_impl<ListType<Ts...>, ListType<Us...>, TypeLists...>
        {
            using Type = mconcat_impl<ListType<Ts..., Us...>, TypeLists...>::Type;
        };
    } // namespace _mconcat

    template<class ... TypeLists>
    using mconcat = _mconcat::mconcat_impl<TypeLists...>::Type;

    template<class ... Ts>
    struct typelist {};

    template<template<class...> class T, class... Args>
    concept valid_specialization = requires { typename T<Args...>; };

    template<class ... Ts>
    struct base_tuple
    {
        template<class ... Args>
        base_tuple(Args&& ... args) : _tuple{ std::tuple{std::forward<Args>(args)...} } {}

        template<class Self, class Func>
        constexpr decltype(auto) apply(this Self&& self, Func&& func) 
            noexcept(noexcept(std::apply(func, std::forward<Self>(self)._tuple)))
        {
            return std::apply(func, std::forward<Self>(self)._tuple);
        }

        std::tuple<Ts...> _tuple;
    };

    template<class ... Args>
    base_tuple(Args&& ... args) -> base_tuple<std::remove_cvref_t<Args>...>;

} // namespace vke::exec