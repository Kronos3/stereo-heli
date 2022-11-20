//
// Created by tumbar on 3/31/22.
//

#ifndef HELI_FRAMEPIPEIMPL_H
#define HELI_FRAMEPIPEIMPL_H

#include <Heli/FramePipe/FramePipeComponentAc.hpp>
#include <Heli/Cam/CamFrame.hpp>

#include <Os/Mutex.hpp>

namespace Heli
{
    class FramePipe : public FramePipeComponentBase
    {
    public:
        explicit FramePipe(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:
        void frame_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;
        void ready_handler(NATIVE_INT_TYPE portNum) override;

        Os::Mutex ready_mutex;
        bool m_frame_ready[NUM_FRAME_INPUT_PORTS];
    };
}

#endif //HELI_FRAMEPIPEIMPL_H
