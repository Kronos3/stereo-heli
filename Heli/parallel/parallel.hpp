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
    class QuitException : std::exception
    {
    };

    class Awaitable
    {
    public:
        Awaitable() : ready(false)
        {}

        Awaitable(const Awaitable& a)
                : ready(a.ready)
        {
        }

        void reset()
        {
            ready = false;
        }

        void await()
        {
            std::unique_lock<std::mutex> lock(mutex);
            while (!ready)
            {
                cv.wait(lock);
            }
        }

        void signal()
        {
            std::unique_lock<std::mutex> lock(mutex);
            ready = true;
            cv.notify_all();
        }

    PRIVATE:
        bool ready;

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

        void quit()
        {
            quiting = true;
            condition.notify_all();
        }

    private:
        bool quiting = false;

        std::mutex mutex;
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
                    (*function)(m_queue.pop());
                    awaiter.signal();
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

        void join()
        {
            m_queue.quit();
            m_thread.join();
        }

        void set(std::function<void(A)> f)
        {
            function = std::make_unique<std::function<void(A)>>(f);
        }

        Awaitable& feed(const A& a)
        {
            awaiter.reset();
            m_queue.push(a);
            return awaiter;
        }

        ~ParallelThread()
        {
            join();
        }

    private:
        std::unique_ptr<std::function<void(A)>> function;
        Awaitable awaiter;
        Queue<A> m_queue;
        std::thread m_thread;
    };

    template<U32 n, typename A>
    class Parallelize
    {
    public:
        explicit Parallelize(std::function<void(A)> f)
        {
            for (auto& i : m_threads)
            {
                i.set(f);
            }
        }

        template<U32 i>
        Awaitable& feed(A&& args)
        {
            static_assert(i < n);
            return m_threads[i].feed(args);
        }

    PRIVATE:
        std::array<ParallelThread<A>, n> m_threads;
    };
}

#endif //STEREO_HELI_PARALLEL_HPP
