#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <functional>
#include <ranges>
#include <algorithm>

namespace vke{

    template<class T>
    struct Getter
    {
        using Data = T;
        using Func = std::function<Data()>;

        Getter() = delete;

        template<class U>
            requires std::convertible_to<U, Data>
        Getter(U&& u) : f_{[data = static_cast<Data>(u)]() -> Data { return data; } } {}

        template<class F>
            requires std::constructible_from<Func, F>
        Getter(F&& f) : f_{ std::forward<F>(f) } {}

        inline Data operator()() const { return f_(); }
        inline Data operator()(Data defaultValue) const { return f_ ? f_() : defaultValue; }

        Func f_{};
    };
    
    template<class T, class ... Args>
    struct Getter<T(Args...)>
    {
        using Data = T;
        using Func = std::function<Data(Args...)>;

        Getter() = delete;

        template<class U>
            requires std::convertible_to<U, Data>
        Getter(U&& u) : f_{[data = static_cast<Data>(u)](Args...) -> Data { return data; } } {}

        template<class F>
            requires std::constructible_from<Func, F>
        Getter(F&& f) : f_{ std::forward<F>(f) } {}

        inline Data operator()(Args ... args) const { return f_( static_cast<Args>(args)... ); }
        inline Data operator()(Data defaultValue, Args ... args) const { return f_ ? f_( static_cast<Args>(args)... ) : defaultValue; }

        Func f_{};
    };

    template<class T>
    struct Checker
    {
        using Data = T;

        Checker() = delete;

        template<std::ranges::input_range List>
        Checker(List&& list, bool validList = true)
            requires std::same_as< std::remove_pointer_t<decltype(std::ranges::data(list))>, Data>
            : f_{ [list_ = static_cast<List>(list), bValid = validList]( const Data& value) -> bool
            { return (std::ranges::find(list_, value) != std::ranges::end(list_)) == bValid; } } {}

        template<std::ranges::input_range List, class ... Args>
        Checker(List&& list, Args&& ... args)
            requires std::invocable<decltype(std::ranges::find), List, const Data&, Args...> &&
                std::equality_comparable_with<std::invoke_result_t<decltype(std::ranges::find), List, const Data&, Args...>, decltype(std::ranges::end(list))>
            : f_{ [list = static_cast<List>(list), ...args = static_cast<Args>(args) ]( const Data& value) -> bool
            { return std::ranges::find(list, value, args...) != std::ranges::end(list); } } {}
            
        template<class F>
            requires std::constructible_from<std::function<bool(const Data&)>, F>
        Checker(F&& f) : f_{ std::forward<F>(f) } {}

        Checker(bool defaultValue) : f_{ [defaultValue](const Data&) { return defaultValue; } } {}

        inline bool operator()(const Data& v) const { return f_(v); }

        std::function<bool(const Data&)> f_{};
    };

    template<class Bits>
    struct Checker<vk::Flags<Bits>>
    {
        using Data = vk::Flags<Bits>;
        using Func = std::function<bool(Bits)>;
        using FilterFunc = std::function<Data(Data)>;

        Checker() = delete;

        Checker(Data flags, bool validList = true)
            : f_{ [flags, validList]( Bits value) -> bool { return static_cast<bool>(flags & value) == validList; } },
            filter_{ [flags, validList](Data data) -> Data { return validList ? (data & flags) : ( data & ~flags ); } } {}

        template<class F>
            requires std::constructible_from<Func, F>
        Checker(F&& f) : f_{ std::forward<F>(f) },
            filter_{ [this](Data data) -> Data 
                {  
                    Data r = static_cast<Data>(0);
                    for(Bits b : std::ranges::views::iota(0, 32) 
                        | std::ranges::views::transform([](uint32_t index){ return static_cast<Bits>( 1 < index); })
                        | std::ranges::views::filter(f_))
                    {
                        r |= b;
                    }

                    return r;
                } } {}

        Checker(bool defaultValue) : f_{ [defaultValue](const Bits&) { return defaultValue; } },
            filter_{ [defaultValue](Data data) -> Data{ return defaultValue ? data : static_cast<Data>(0); } } {}

        inline bool operator()(Bits v) const { return f_(v); }
        inline Data operator()(Data data) const { return filter_(data); }

        Func f_{};
        FilterFunc filter_{};
    };

    template<class T>
    struct Selecter
    {
        using Data = T;
        using Func = std::function<uint32_t(const Data&)>;

        Selecter() = delete;

        template<class F>
            requires std::constructible_from<Func, F>
        Selecter(F&& f) : f_{ std::forward<F>(f) } {}

        template<class F, class C>
            requires std::constructible_from<Func, F> &&
                std::constructible_from<Checker<Data>, C>
                    Selecter(F&& f, C&& checker_) : f_{ std::forward<F>(f) }, checker{ std::forward<C>(checker_) } {}

        inline Data operator()(std::span<const std::remove_cvref_t<Data> > list) const 
        {
            auto filter = list | std::ranges::views::filter(checker);

            if(!f_)
                return filter.front();

            return std::ranges::max( filter, std::ranges::less{}, f_ );
        }

        inline Data operator()(std::span<const std::remove_cvref_t<Data> > list, Data defaultValue) const 
        {
            auto filter = list | std::ranges::views::filter(checker);

            if(!f_)
                return defaultValue;

            return std::ranges::max( filter, std::ranges::less{}, f_ );
        }

        Checker<Data> checker{true};
        Func f_{};
    };

    template<class Bits>
    struct Selecter<vk::Flags<Bits>>
    {
        using Data = vk::Flags<Bits>;
        using Func = std::function<uint32_t(const Bits&)>;

        Selecter() = delete;

        template<class F>
            requires std::constructible_from<Func, F>
        Selecter(F&& f) : f_{ std::forward<F>(f) } {}

        template<class F, class C>
            requires std::constructible_from<Func, F> &&
                std::constructible_from<Checker<Data>, C>
        Selecter(F&& f, C&& checker) : f_{ std::forward<F>(f) } {}

        inline Bits operator()(Data bitList) const 
        {
            auto filter = std::ranges::views::iota(0u, 32u) 
                | std::ranges::views::transform([](uint32_t index) -> Bits { return static_cast<Bits>( 1 << index ); })
                | std::ranges::views::filter([&](Bits b){ return (bitList & b) && checker(b); });

            if(!f_)
                return filter.front();

            return std::ranges::max(filter, std::ranges::less{}, f_ );
        }
        
        inline Bits operator()(Data bitList, Bits defaultValue) const 
        {
            auto filter = std::ranges::views::iota(0u, 32u) 
                | std::ranges::views::transform([](uint32_t index) -> Bits { return static_cast<Bits>( 1 << index ); })
                | std::ranges::views::filter([&](Bits b){ return (bitList & b) && checker(b); });

            if(!f_)
                return defaultValue;

            return std::ranges::max(filter, std::ranges::less{}, f_ );
        }

        Checker<Data> checker{true};
        Func f_{};
    };
}