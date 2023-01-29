//
// Created by tumbar on 12/28/22.
//

#ifndef STEREO_HELI_INTERVAL_TIMER_HPP
#define STEREO_HELI_INTERVAL_TIMER_HPP

#include <Heli/IntervalTimer/IntervalTimerComponentAc.hpp>
#include "Mutex.hpp"
#include "Fw/Time/Time.hpp"

namespace Heli
{
    class IntervalTimer : public IntervalTimerComponentBase
    {
    public:

        explicit IntervalTimer(const char* componentName);

        void init(
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

    PRIVATE:
        static void main_routine(void* this_);
        void main();

        void start_handler(NATIVE_INT_TYPE portNum,
                           Fw::Time& interval,
                           Fw::Time& value) override;

        void stop_handler(NATIVE_INT_TYPE portNum) override;

    PRIVATE:
        Os::Mutex m_mutex;
        Os::Task m_task;

        Fw::Time m_interval;
        Fw::Time m_value;

        Svc::TimerVal m_timer;

        volatile bool m_running;
    };
}

#endif //STEREO_HELI_INTERVAL_TIMER_HPP
