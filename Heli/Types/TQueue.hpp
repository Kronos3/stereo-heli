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
        TQueue();

        T pop();
        void push(T item);

        template <class... Args>
        void emplace(Args&&... args);

        bool empty() const;
        bool full() const;
    };

    template<typename T, U32 size>
    template<class... Args>
    void TQueue<T, size>::emplace(Args &&... args)
    {
        mut.lock();
        buf[end++] = T(args...);
        end %= size;
        mut.unlock();
    }

    template<typename T, U32 size>
    void TQueue<T, size>::push(T item)
    {
        mut.lock();
        buf[end++] = item;
        end %= size;
        mut.unlock();
    }

    template<typename T, U32 size>
    T TQueue<T, size>::pop()
    {
        mut.lock();
        T item = buf[start++];
        start %= size;
        mut.unlock();
        return item;
    }

    template<typename T, U32 size>
    bool TQueue<T, size>::empty() const
    {
        mut.lock();
        bool o = start == end;
        mut.unlock();
        return o;
    }

    template<typename T, U32 size>
    bool TQueue<T, size>::full() const
    {
        mut.lock();
        bool o = ((start - 1) % size) == end;
        mut.unlock();
        return o;
    }

    template<typename T, U32 size>
    TQueue<T, size>::TQueue()
    : start(0), end(0), buf{}
    {
    }
}

#endif //STEREO_HELI_TQUEUE_HPP
