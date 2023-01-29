//
// Created by tumbar on 12/28/22.
//

#include <Heli/IntervalTimer/IntervalTimer.hpp>

#include <sys/timerfd.h>
#include <unistd.h>
#include <cerrno>
#include "Logger.hpp"

namespace Heli
{
    IntervalTimer::IntervalTimer(const char* componentName)
            : IntervalTimerComponentBase(componentName),
              m_running(false)
    {
    }

    void IntervalTimer::init(NATIVE_INT_TYPE instance)
    {
        IntervalTimerComponentBase::init(instance);
    }

    void IntervalTimer::main_routine(void* this_)
    {
        static_cast<IntervalTimer*>(this_)->main();
    }

    void IntervalTimer::start_handler(NATIVE_INT_TYPE portNum,
                                      Fw::Time &interval,
                                      Fw::Time &value)
    {
        m_mutex.lock();
        if (m_running)
        {
            log_WARNING_HI_AlreadyRunning();
            m_mutex.unlock();
            return;
        }

        m_interval = interval;
        m_value = value;
        m_running = true;
        m_mutex.unlock();

        Os::TaskString name("IntervalTimer");
        m_task.start(name, IntervalTimer::main_routine, this);
    }

    void IntervalTimer::stop_handler(NATIVE_INT_TYPE portNum)
    {
        m_mutex.lock();
        if (!m_running)
        {
            log_WARNING_HI_NotRunning();
            m_mutex.unlock();
            return;
        }

        m_interval = Fw::Time();
        m_value = Fw::Time();
        m_running = false;
        m_mutex.unlock();

        m_task.join(nullptr);
    }

    void IntervalTimer::main()
    {
        FW_ASSERT(m_running);

        int fd;
        struct itimerspec itval;

        /* Create the timer */
        fd = timerfd_create(CLOCK_REALTIME, 0);

        m_mutex.lock();
        itval.it_interval.tv_sec = m_interval.getSeconds();
        itval.it_interval.tv_nsec = m_interval.getUSeconds() * 1000;
        itval.it_value.tv_sec = m_value.getSeconds();
        itval.it_value.tv_nsec = m_value.getUSeconds() * 1000;

        timerfd_settime(fd, 0, &itval, nullptr);
        m_mutex.unlock();

        log_ACTIVITY_LO_Started();

        while (true)
        {
            U64 missed;
            int ret = read(fd, &missed, sizeof(missed));
            if (-1 == ret)
            {
                Fw::Logger::logMsg(
                        "timer read error: %s\n",
                        reinterpret_cast<POINTER_CAST>(strerror(errno)));
            }

            if (missed > 1)
            {
                log_WARNING_LO_MissedCycles(missed - 1);
            }

            m_mutex.lock();
            bool running = m_running;
            m_mutex.unLock();

            if (!running)
            {
                break;
            }

            m_timer.take();
            CycleOut_out(0, m_timer);
        }

        itval.it_interval.tv_sec = 0;
        itval.it_interval.tv_nsec = 0;
        itval.it_value.tv_sec = 0;
        itval.it_value.tv_nsec = 0;

        timerfd_settime(fd, 0, &itval, nullptr);
        close(fd);

        log_ACTIVITY_LO_Stopped();
        m_running = false;
    }
}
