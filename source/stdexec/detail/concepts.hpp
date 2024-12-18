#pragma once

#include <type_traits>
#include <concepts>

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

} // namespace vke::exec