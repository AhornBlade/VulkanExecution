#pragma once

#include "receiver.hpp"
#include "operation_state.hpp"
#include "sender.hpp"

namespace vke::exec
{
    namespace _basic
    {
        struct get_data {
            template <class _Data>
            _Data&& operator()(auto, _Data&& _data, auto&&...) const noexcept 
            {
                return static_cast<_Data&&>(_data);
            }
        };

        template<class Tag>
        struct impls_for;

        template <class Tag, class Data, class... Child>
        struct sender_traits_helper
        {
            using tag = Tag;
            using data = Data;
            using children = typelist<Child...>;
        };

        template<class Sndr>
        using sender_traits = decltype(std::apply(
            []<class Tag, class Data, class... Child>(Tag, Data, Child...) -> decltype(auto)
            {
                return sender_traits_helper<Tag, Data, Child...>{};
            }, std::declval<Sndr>()._tuple));

        template<class Sndr>
        using tag_of_t = sender_traits<Sndr>::tag;
        
        template<class Sndr, class Rcvr>
        using state_type = std::decay_t<std::invoke_result_t<
            decltype(impls_for<tag_of_t<Sndr>>::get_state), Sndr, Rcvr&>>;

        template<class Index, class Sndr, class Rcvr>
        using env_type = std::invoke_result_t<
            decltype(impls_for<tag_of_t<Sndr>>::get_env), Index,
            state_type<Sndr, Rcvr>&, const Rcvr&>;

        template<class Sndr, class Rcvr>
        struct basic_state 
        {
            basic_state(Sndr&& sndr, Rcvr&& rcvr) 
                noexcept(noexcept(impls_for<tag_of_t<Sndr>>::get_state(std::forward<Sndr>(sndr), rcvr)))
            : rcvr(std::move(rcvr))
            , state(impls_for<tag_of_t<Sndr>>::get_state(std::forward<Sndr>(sndr), rcvr)) { }

            Rcvr rcvr;
            state_type<Sndr, Rcvr> state;
        };

        template<class Sndr, class Rcvr, class Index>
            requires valid_specialization<env_type, Index, Sndr, Rcvr>
        struct basic_receiver { 
            using receiver_concept = receiver_t;

            using tag_t = tag_of_t<Sndr>;
            using state_t = state_type<Sndr, Rcvr>;
            static constexpr const auto& complete = impls_for<tag_t>::complete;

            template<class... Args>
                requires std::invocable<decltype(complete), Index, basic_state<Sndr, Rcvr>*, set_value_t, Args...>
            void set_value(Args&&... args) && noexcept 
            {
                complete(Index(), op, set_value_t(), std::forward<Args>(args)...);
            }

            template<class Error>
                requires std::invocable<decltype(complete), Index, basic_state<Sndr, Rcvr>*, set_error_t, Error>
            void set_error(Error&& err) && noexcept 
            {
                complete(Index(), op, set_error_t(), std::forward<Error>(err));
            }

            void set_stopped() && noexcept
                requires std::invocable<decltype(complete), Index, basic_state<Sndr, Rcvr>*, set_stopped_t> 
            {
                complete(Index(), op, set_stopped_t());
            }

            auto get_env() const noexcept -> env_type<Index, Sndr, Rcvr> 
            {
                return impls_for<tag_t>::get_env(Index(), op->state, op->rcvr);
            }

            basic_state<Sndr, Rcvr>* op;
        };

        template<class Sndr>
        using indices_for = std::remove_reference_t<Sndr>::indices_for;

        template<class Sndr, class Rcvr>
        using connect_all_result = std::invoke_result_t<
            decltype(impls_for<tag_of_t<Sndr>>::connect), basic_state<Sndr, Rcvr>*, Sndr, indices_for<Sndr>>;

        template<class Sndr, class Rcvr>
            requires valid_specialization<state_type, Sndr, Rcvr> &&
                    valid_specialization<connect_all_result, Sndr, Rcvr>
        struct basic_operation : basic_state<Sndr, Rcvr> 
        { 
            using operation_state_concept = operation_state_t;
            using tag_t = tag_of_t<Sndr>;

            connect_all_result<Sndr, Rcvr> inner_ops; 

            basic_operation(Sndr&& sndr, Rcvr&& rcvr) 
            noexcept(noexcept(impls_for<tag_t>::connect(this, std::forward<Sndr>(sndr), indices_for<Sndr>())))
            : basic_state<Sndr, Rcvr>(std::forward<Sndr>(sndr), std::move(rcvr))
            , inner_ops(impls_for<tag_t>::connect(this, std::forward<Sndr>(sndr), indices_for<Sndr>()))
            {}

            void start() & noexcept 
            {
                std::apply([&](auto& ... ops)
                {
                    impls_for<tag_t>::start(this->state, this->rcvr, ops...);
                }, inner_ops);
            }
        };

        template<class Tag, class Data, class... Child>
        struct basic_sender
        {
            using sender_concept = sender_t;
            using indices_for = std::index_sequence_for<Child...>;
            using tuple_type = decayed_tuple<Tag, Data, Child...>;

            template<class _Tag, class _Data, class ... _Child>
            basic_sender(_Tag tag, _Data&& data, _Child&& ... child)
                : _tuple{ tag, std::forward<_Data>(data), std::forward<_Child>(child)... } {}

            decltype(auto) get_env() const noexcept {
                return std::apply([](auto, auto& data, auto& ... child)
                {
                    return impls_for<Tag>::get_attrs(data, child...);
                }, _tuple);
            }

            template<decays_to<basic_sender> Self, receiver Rcvr>
            auto connect(this Self&& self, Rcvr rcvr) 
                noexcept(std::is_nothrow_constructible_v<basic_operation<Self, Rcvr>, Self, Rcvr>)
                -> basic_operation<Self, Rcvr> 
            {
                return {std::forward<Self>(self), std::move(rcvr)};
            }

            template<decays_to<basic_sender> Self, class Env>
            constexpr auto get_completion_signatures(this Self&& self, Env&& env) noexcept
            {
                return std::apply([&](auto, auto& data, Child& ... child)
                {
                    return impls_for<Tag>::get_completion_signatures(std::forward<Env>(env), data, 
                        completion_signatures_for<Child, Env>{}...);
                }, self._tuple);
            }
            
            template<std::size_t I, decays_to<basic_sender> Self>
            decltype(auto) get(this Self&& self) noexcept
            {
                return ::std::get<I>(std::forward_like<Self>(self._tuple));
            }

            tuple_type _tuple;
        };

        template<class _Tag, class _Data, class ... _Child>
        basic_sender(_Tag tag, _Data&& data, _Child&& ... child) -> basic_sender<_Tag, _Data, _Child...>;
        
        struct default_impls
        {
            static constexpr auto get_attrs = 
                [](const auto&, const auto&... child) noexcept -> decltype(auto) {
                if constexpr (sizeof...(child) == 1)
                    return exec::get_env(child...);
                else
                    return empty_env{};
                };
            static constexpr auto get_env = 
                [](auto, auto&, const auto& rcvr) noexcept -> decltype(auto) {
                    return exec::get_env(rcvr);
                };
            static constexpr auto get_state = 
                []<class Sndr, class Rcvr>(Sndr&& sndr, Rcvr& rcvr) noexcept -> decltype(auto) {
                    return std::apply(get_data{}, std::forward<Sndr>(sndr)._tuple);
                };
            static constexpr auto start = 
                [](auto&, auto&, auto&... ops) noexcept -> void {
                    (exec::start(ops), ...);
                };
            static constexpr auto complete = 
                []<class Index, class Sndr, class Rcvr, class Tag, class... Args>(
                    Index index, basic_state<Sndr, Rcvr>* op, Tag, Args&&... args) noexcept
                        -> void requires std::invocable<Tag, Rcvr, Args...> 
                {
                    static_assert(index == 0, "I don't know how to complete this operation.");
                    Tag()(std::move(op->rcvr), std::forward<Args>(args)...);
                };
            static constexpr auto get_completion_signatures = 
                []<class ... Sigs>(auto&& env, auto& data, Sigs ... child_sigs)
                {
                    if constexpr(sizeof...(Sigs) == 0)
                    {
                        return completion_signatures<>{};
                    }
                    else
                    {
                        return _munique_remove_empty<mconcat<Sigs...>>{};
                    }
                };
            static constexpr auto connect = 
                []<class Sndr, class Rcvr, size_t... Is>(
                basic_state<Sndr, Rcvr>* op, Sndr&& sndr, std::index_sequence<Is...>)
                    -> decltype(auto) 
                {
                    return std::apply(
                    [&]<class ... Child>(auto&, auto& data, Child& ... child)
                    {
                        return std::tuple{exec::connect(
                            std::forward<Child>(child),
                            basic_receiver<Sndr, Rcvr, std::integral_constant<size_t, Is>>{op})...};
                    }, std::forward<Sndr>(sndr)._tuple);
                };
        };

        template<class Tag>
        struct impls_for : default_impls {};

    }// namespace _basic

    template<class Tag, class ... Ts>
    struct sender_adaptor_closure
    {
        template<class _Tag, class ... _Ts>
        constexpr sender_adaptor_closure(_Tag, _Ts&& ... args) 
            : _tuple{std::forward<_Ts>(args)...} {}

        template<sender Sndr, decays_to<sender_adaptor_closure> Self>
        friend sender decltype(auto) operator|(Sndr&& sndr, Self&& self) noexcept
        {
            return std::apply([&sndr]<class ... Args>(Args&& ... args)
            {
                return Tag{}(std::forward<Sndr>(sndr), std::forward<Args>(args)...);
            }, std::forward<Self>(self)._tuple);
        }

        std::tuple<Ts...> _tuple;
    };

    template<class _Tag, class ... _Ts>
    sender_adaptor_closure(_Tag, _Ts&& ... args) -> sender_adaptor_closure<_Tag, _Ts...>;

}// namespace vke::exec