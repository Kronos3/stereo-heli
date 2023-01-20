//
// Created by tumbar on 1/9/23.
//

#ifndef STEREO_HELI_TQUEUE_HPP
#define STEREO_HELI_TQUEUE_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Os/Mutex.hpp>

namespace Types
{
    template<typename T, U32 size>
    class TQueue
    {
        U32 start;
        U32 end;
        T buf[size];
        mutable Os::Mutex mut;

    public:
        TQueue()
        : start(0), end(0), buf{}
        {
        }

        T pop()
        {
            mut.lock();
            T item = buf[start++];
            start %= size;
            mut.unlock();
            return item;
        }

        void push(T item)
        {
            mut.lock();
            buf[end++] = item;
            end %= size;
            mut.unlock();
        }

        template <class... Args>
        void emplace(Args&&... args)
        {
            mut.lock();
            buf[end++] = T(args...);
            end %= size;
            mut.unlock();
        }

        bool empty() const
        {
            mut.lock();
            bool o = start == end;
            mut.unlock();
            return o;
        }

        bool full() const
        {
            mut.lock();
            bool o = ((start - 1) % size) == end;
            mut.unlock();
            return o;
        }
    };
}

#endif //STEREO_HELI_TQUEUE_HPP
