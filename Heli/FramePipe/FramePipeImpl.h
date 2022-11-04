//
// Created by tumbar on 3/31/22.
//

#ifndef HELI_FRAMEPIPEIMPL_H
#define HELI_FRAMEPIPEIMPL_H

#include <Rpi/FramePipe/FramePipeComponentAc.hpp>
#include <Os/Mutex.hpp>

namespace Rpi
{
    class FramePipeImpl : public FramePipeComponentBase
    {
    public:
        explicit FramePipeImpl(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:
        void frame_handler(NATIVE_INT_TYPE portNum, CamFrame *frame) override;
        void ready_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context) override;

        Os::Mutex ready_mutex;
        bool m_frame_ready[NUM_FRAME_INPUT_PORTS];
    };
}

#endif //HELI_FRAMEPIPEIMPL_H
