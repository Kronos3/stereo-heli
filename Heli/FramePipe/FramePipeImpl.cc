//
// Created by tumbar on 3/31/22.
//

#include "FramePipeImpl.h"
#include <Fw/Types/Assert.hpp>

namespace Rpi
{

    FramePipeImpl::FramePipeImpl(const char* compName)
    : FramePipeComponentBase(compName), m_frame_ready{true}
    {
    }

    void FramePipeImpl::init(NATIVE_INT_TYPE instance)
    {
        FramePipeComponentBase::init(instance);
    }

    void FramePipeImpl::frame_handler(NATIVE_INT_TYPE portNum, CamFrame* frame)
    {
        ready_mutex.lock();
        if (!m_frame_ready[portNum])
        {
            // TODO(tumbar) Tlm on dropped frames
            frame->decref();
        }
        else
        {
            // Component is ready
            // Send the frame through
            m_frame_ready[portNum] = false;
            frameOut_out(portNum, frame);
        }
        ready_mutex.unLock();
    }

    void FramePipeImpl::ready_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context)
    {
        FW_ASSERT(!m_frame_ready[portNum], portNum);
        m_frame_ready[portNum] = true;
    }
}
