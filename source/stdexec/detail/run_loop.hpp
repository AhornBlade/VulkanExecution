#pragma once

#include "scheduler.hpp"
#include "sender.hpp"
#include "basic.hpp"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace vke::exec
{
    namespace _run_loop
    {
        struct run_loop;

        struct run_loop_tag
        {
            using Tag = run_loop_tag;
        };

        struct run_loop_env
        {
            template<completion_tag T>
            auto query(get_completion_scheduler_t<T>) const noexcept;

            run_loop* _loop;
        };
        
        struct run_loop_executor_base
        {
            run_loop_executor_base(run_loop* loop) : _loop{loop} {}

            virtual void execute() = 0;

            void push_to_loop();

            run_loop* _loop;
        };

        template<class Rcvr>
        struct run_loop_executor : run_loop_executor_base
        {
            run_loop_executor(run_loop* loop, Rcvr* rcvr) : 
                run_loop_executor_base{loop}, _rcvr{rcvr} {}

            void execute()
            {
                if (get_stop_token(*_rcvr).stop_requested()) 
                {
                    set_stopped(std::move(*_rcvr));
                } else 
                {
                    set_value(std::move(*_rcvr));
                }
            }

            Rcvr* _rcvr;
        };
    } // namespace _run_loop
    
    namespace _basic
    {
        template<>
        struct impls_for<_run_loop::run_loop_tag> : default_impls 
        {
            static constexpr auto get_attrs = 
                [](const auto& data, const auto&... child) noexcept -> decltype(auto) {
                    return _run_loop::run_loop_env{data};
                };

            static constexpr auto get_completion_signatures = 
                [](auto&& env, auto& data, auto ... child_sigs) 
                    -> completion_signatures<set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>
                {
                    return {};
                };

            static constexpr auto get_state = 
                []<class Sndr, class Rcvr>(Sndr&& sndr, Rcvr& rcvr) noexcept -> decltype(auto) 
                {
                    return _run_loop::run_loop_executor<Rcvr>{
                        default_impls::get_state(std::forward<Sndr>(sndr), rcvr), &rcvr};
                };

            static constexpr auto start = 
                [](auto& state, auto& rcvr, auto&... ops) noexcept -> void 
                {
                    try {
                        state.push_to_loop();
                    } catch(...) {
                        set_error(std::move(rcvr), std::current_exception());
                    }
                };
        };
    }

    namespace _run_loop
    {
        class run_loop;

        struct run_loop_scheduler
        {
            using scheduler_concept = scheduler_t;

            sender auto schedule() noexcept
            {
                return _basic::basic_sender{run_loop_tag{}, auto(loop)};
            }

            constexpr bool operator==(const run_loop_scheduler& other) const noexcept
            {
                return loop == other.loop;
            }

            run_loop* loop;
        };

        class run_loop 
        {
        public:
            run_loop() = default;
            virtual ~run_loop() noexcept = default;

            run_loop_scheduler get_scheduler()
            {
                return {this};
            }
            void run()
            {
                while(run_loop_executor_base* exec = pop_front())
                {
                    exec->execute();
                }
            }
            virtual void finish()
            {
                is_finished = true;
            }

            virtual run_loop_executor_base* pop_front()
            {
                if(is_finished || executors.empty())
                {
                    return nullptr;
                }

                run_loop_executor_base* exec = executors.front();
                executors.pop();
                return exec;
            }

            virtual void push_back(run_loop_executor_base* p_executor)
            {
                executors.push(p_executor);
            }

        protected:
            std::queue<run_loop_executor_base*> executors;
            bool is_finished = false;
        };

        template<completion_tag T>
        auto run_loop_env::query(get_completion_scheduler_t<T>) const noexcept
        {
            return _loop->get_scheduler();
        }
        
        inline void run_loop_executor_base::push_to_loop()
        {
            _loop->push_back(this);
        }

        class synchronized_run_loop : public run_loop
        {
        public:
            virtual run_loop_executor_base* pop_front()
            {
                std::unique_lock lock{_mutex};
                _cv.wait(lock, [this]{ return is_finished || (!executors.empty()); });
                return run_loop::pop_front();
            }
            
            virtual void push_back(run_loop_executor_base* p_executor)
            {
                {
                    std::unique_lock lock{_mutex};
                    run_loop::push_back(p_executor);
                }
                _cv.notify_one();
            }
            
            virtual void finish()
            {
                run_loop::finish();
                _cv.notify_all();
            }

        private:
            mutable std::mutex _mutex;
            mutable std::condition_variable _cv;
        };
    }

    using _run_loop::run_loop;
    using _run_loop::synchronized_run_loop;
    using _run_loop::run_loop_scheduler;

    namespace _thread_pool
    {
        class thread_pool
        {
        public:
            explicit thread_pool(uint32_t thread_count)
            {
                _threads.reserve(thread_count);
                for(uint32_t i = 0; i < thread_count; i++)
                {
                    _threads.emplace_back([this]
                    {
                        _loop.run();
                    });
                }
            }

            scheduler auto get_scheduler() noexcept
            {
                return _loop.get_scheduler();
            }

        private:
            std::vector<std::jthread> _threads;
            synchronized_run_loop _loop;
        };
    };

    using _thread_pool::thread_pool;

} // namespace vke::exec