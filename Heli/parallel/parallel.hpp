//
// Created by tumbar on 3/4/23.
//

#ifndef STEREO_HELI_PARALLEL_HPP
#define STEREO_HELI_PARALLEL_HPP

#include <Fw/Types/BasicTypes.hpp>

#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <queue>

namespace libparallel
{
    template <U32 n, typename R, typename... A>
    class Parallelize
    {
        using Arguments = std::tuple<A...>;
        using Queue = std::queue<Arguments>;

    public:
        explicit Parallelize(const std::function<R&(A...)>& f)
        : m_f(f)
        {
            for (U32 i = 0; i < n; i++)
            {
                auto& q = m_queues.emplace_back();
                m_threads.emplace_back(run, this, q);
            }
        }

        template<U32 i>
        R feed(A... args)
        {
            static_assert(i < n);
        }

    PRIVATE:
        static void run(Parallelize<n, R, A...>* t, Queue& queue)
        {

        };

        std::function<R&(A...)> m_f;
        std::vector<std::thread> m_threads;
        std::vector<Queue> m_queues;
    };
}

#endif //STEREO_HELI_PARALLEL_HPP
