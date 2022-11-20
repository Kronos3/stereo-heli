//
// Created by tumbar on 3/31/22.
//

#include <Heli/FramePipe/FramePipe.hpp>
#include <Fw/Types/Assert.hpp>

namespace Heli
{

    FramePipe::FramePipe(const char* compName)
    : FramePipeComponentBase(compName), m_frame_ready{true}
    {
    }

    void FramePipe::init(NATIVE_INT_TYPE instance)
    {
        FramePipeComponentBase::init(instance);
    }

    void FramePipe::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        ready_mutex.lock();
        if (!m_frame_ready[portNum])
        {
            // TODO(tumbar) Tlm on dropped frames
            incdec_out(0, frameId, ReferenceCounter::DECREMENT);
        }
        else
        {
            // Component is ready
            // Send the frame through
            m_frame_ready[portNum] = false;
            frameOut_out(portNum, frameId);
        }
        ready_mutex.unLock();
    }

    void FramePipe::ready_handler(NATIVE_INT_TYPE portNum)
    {
        FW_ASSERT(!m_frame_ready[portNum], portNum);
        m_frame_ready[portNum] = true;
    }
}
