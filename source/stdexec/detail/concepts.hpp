#pragma once

#include <concepts>
#include <tuple>
#include <variant>

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

    struct empty_type {};

    namespace _munique
    {
        template<class TypeList1, class TypeList2>
        struct munique_remove_empty_impl;

        template<template<class ...>typename ListType, class ... Ts>
        struct munique_remove_empty_impl<ListType<Ts...>, ListType<>>
        {
            using Type = ListType<Ts...>;
        };

        template<template<class ...>typename ListType, class ... Ts, class U, class ... Us>
        struct munique_remove_empty_impl<ListType<Ts...>, ListType<U, Us...>>
        {
            using Type = munique_remove_empty_impl< std::conditional_t<
                one_of<U, Ts...> || std::same_as<U, empty_type>, 
                ListType<Ts...>, 
                ListType<Ts..., U>>, ListType<Us...>>::Type;
        };

        template<class TypeList>
        struct munique_remove_empty_helper;

        template<template<class ...>typename ListType, class ... Ts>
        struct munique_remove_empty_helper<ListType<Ts...>>
        {
            using Type = munique_remove_empty_impl<ListType<>, ListType<Ts...>>::Type;
        };
    }; // namespace _munique

    template<class TypeList>
    using _munique_remove_empty = _munique::munique_remove_empty_helper<TypeList>::Type;

    namespace _mconcat
    {
        template<class ... TypeLists>
        struct mconcat_helper;

        template<template<class...> typename ListType, class ... Ts>
        struct mconcat_helper<ListType<Ts...>>
        {
            using Type = ListType<Ts...>;
        };

        template<template<class...> typename ListType, class ... Ts, class ... Us, class ... TypeLists>
        struct mconcat_helper<ListType<Ts...>, ListType<Us...>, TypeLists...>
        {
            using Type = mconcat_helper<ListType<Ts..., Us...>, TypeLists...>::Type;
        };

        template<class ... TypeLists>
        using mconcat = mconcat_helper<TypeLists...>::Type;

    } // namespace _mconcat

    using _mconcat::mconcat;

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
    using decayed_tuple = std::tuple<std::decay_t<Ts>...>;

    namespace _variant_or_empty
    {
        struct empty_variant {};

        template<class ... Ts>
        struct variant_or_empty
        {
            using Type = std::variant<Ts...>;
        };

        template<>
        struct variant_or_empty<>
        {
            using Type = empty_variant;
        };

        template<class ... Ts>
        using variant_or_empty_t = variant_or_empty<std::decay_t<Ts>...>::Type;

    } // namespace _variant_or_empty

    using _variant_or_empty::empty_variant;
    using _variant_or_empty::variant_or_empty_t;
    using _variant_or_empty::variant_or_empty;
    
} // namespace vke::exec