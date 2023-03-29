//
// Created by tumbar on 3/4/23.
//

#ifndef STEREO_HELI_PARALLEL_HPP
#define STEREO_HELI_PARALLEL_HPP

#include <Fw/Types/BasicTypes.hpp>

#include <mutex>
#include <condition_variable>

#include <type_traits>

#include <iostream>
#include <functional>
#include <memory>
#include <thread>
#include <queue>

namespace libparallel
{
    using Token = I32;

    class QuitException : std::exception
    {
    };

    class Awaitable
    {
    public:
        Awaitable() : ready(false), token(0)
        {}

        Awaitable(const Awaitable& a)
                : ready(a.ready), token(a.token)
        {
        }

        void reset()
        {
            ready = false;
            token = 0;
        }

        Token await()
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!ready)
            {
                cv.wait(lock);
            }

            Token tok = token;
            reset();
            return tok;
        }

        void signal(Token tok)
        {
            std::unique_lock<std::mutex> lock(mutex);
            ready = true;
            token = tok;
            cv.notify_all();
        }

    PRIVATE:
        bool ready;
        Token token;

        std::mutex mutex;
        std::condition_variable cv;
    };

    template<typename T>
    class Queue
    {

    public:
        void push(const T& t)
        {
            std::unique_lock<std::mutex> lock(mutex);
            queue.push(t);
            condition.notify_all();
        }

        T pop()
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (queue.empty())
            {
                condition.wait(lock);
                if (quiting)
                {
                    throw QuitException();
                }
            }

            T r = queue.back();
            queue.pop();
            return r;
        }

        bool empty() const
        {
            std::unique_lock<std::mutex> lock(mutex);
            return queue.empty();
        }

        void clear()
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::queue<T>().swap(queue);
        }

        size_t size() const
        {
            std::unique_lock<std::mutex> lock(mutex);
            return queue.size();
        }

        void quit()
        {
            quiting = true;
            condition.notify_all();
        }

    private:
        bool quiting = false;

        mutable std::mutex mutex;
        std::condition_variable condition;
        std::queue<T> queue;
    };

    template<typename A>
    class ParallelThread
    {
    public:
        ParallelThread()
                : m_thread([=]() {
            while(true)
            {
                try
                {
                    auto p = m_queue.pop();
                    (*function)(p.second);
                    awaiter.signal(p.first);
                }
                catch(QuitException&)
                {
                    break;
                }
                catch(std::exception& e)
                {
                    std::cerr << "Exception occurred in parallel thread" << e.what() << std::endl;
                    break;
                }
            }
        })
        {
        }

        void set(std::function<void(A)> f)
        {
            function = std::make_unique<std::function<void(A)>>(f);
        }

        Awaitable& feed(const A& a, Token tok)
        {
            m_queue.push({tok, a});
            return awaiter;
        }

        ~ParallelThread()
        {
            m_queue.quit();
            m_thread.join();
        }

    private:
        std::unique_ptr<std::function<void(A)>> function;
        Awaitable awaiter;
        Queue<std::pair<Token, A>> m_queue;
        std::thread m_thread;
    };

    template<U32 n, typename A>
    class Parallelize
    {
    public:
        explicit Parallelize(std::function<void(A)> f)
        {
            for (auto& i : m_threads) i.set(f);
        }

        template<U32 i, Token tok = 0>
        Awaitable& feed(A&& args)
        {
            static_assert(i < n, "Thread index is out of range");
            return m_threads[i].feed(args, tok);
        }

        Awaitable& feed(A&& args, U32 i, Token tok = 0)
        {
            FW_ASSERT(i < n, i);
            return m_threads[i].feed(args, tok);
        }

    PRIVATE:
        std::array<ParallelThread<A>, n> m_threads;
    };
}

#endif //STEREO_HELI_PARALLEL_HPP
