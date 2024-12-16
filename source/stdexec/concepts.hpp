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

} // namespace vke::exec