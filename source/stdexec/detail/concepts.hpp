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

    namespace _munique
    {
        template<class TypeList1, class TypeList2>
        struct munique_remove_void_impl;

        template<template<class ...>typename ListType, class ... Ts>
        struct munique_remove_void_impl<ListType<Ts...>, ListType<>>
        {
            using Type = ListType<Ts...>;
        };

        template<template<class ...>typename ListType, class ... Ts, class U, class ... Us>
        struct munique_remove_void_impl<ListType<Ts...>, ListType<U, Us...>>
        {
            using Type = munique_remove_void_impl< std::conditional_t<
                one_of<U, Ts...> || std::is_void_v<U>, 
                ListType<Ts...>, 
                ListType<Ts..., U>>, ListType<Us...>>::Type;
        };

        template<class TypeList>
        struct munique_remove_void_helper;

        template<template<class ...>typename ListType, class ... Ts>
        struct munique_remove_void_helper<ListType<Ts...>>
        {
            using Type = munique_remove_void_impl<ListType<>, ListType<Ts...>>::Type;
        };
    }; // namespace _munique

    template<class TypeList>
    using _munique_remove_void = _munique::munique_remove_void_helper<TypeList>::Type;

    template<class ... Ts>
    struct and_all
    {
        constexpr static bool value = (Ts::value && ...);
    };

    template<class ... Ts>
    struct typelist {};

    template<template<class...> class T, class... Args>
    concept valid_specialization = requires { typename T<Args...>; };

    template<class ... Ts>
    struct base_tuple
    {
        template<class ... Args>
        base_tuple(Args&& ... args) : _tuple{ std::forward<Args>(args)... } {}

        template<class Self, class Func>
        constexpr decltype(auto) apply(this Self&& self, Func&& func) 
            noexcept(noexcept(std::apply(func, std::forward<Self>(self)._tuple)))
        {
            return std::apply(func, std::forward<Self>(self)._tuple);
        }

        std::tuple<Ts...> _tuple;
    };

    template<class ... Args>
    base_tuple(Args&& ... args) -> base_tuple<Args...>;

    template<class ... Ts>
    using decayed_tuple = base_tuple<std::decay_t<Ts>...>;

} // namespace vke::exec