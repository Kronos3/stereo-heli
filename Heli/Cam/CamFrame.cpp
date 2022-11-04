//
// Created by tumbar on 9/21/22.
//

#include <Rpi/Cam/CamFrame.hpp>

namespace Rpi
{

    CamFrame::CamFrame() = default;

    CamFrame::CamFrame(const CamFrameBase &src) : CamFrameBase(src)
    {
    }

    CamFrame::CamFrame(U32 bufId, U8* data, U32 bufSize, U32 width, U32 height, U32 stride, U64 timestamp, I32 plane)
    : CamFrameBase(bufId, reinterpret_cast<POINTER_CAST>(data),
                   bufSize, width, height,
                   stride, timestamp, plane)
    {
    }

    U8* CamFrame::getData() const
    {
        return reinterpret_cast<U8*>(getdata());
    }

    StreamInfo CamFrame::getInfo() const
    {
        StreamInfo info;
        info.width = m_width;
        info.height = m_height;
        info.stride = m_stride;
        return info;
    }

    U64 CamFrame::getTimestamp() const
    {
        return m_timestamp;
    }

    I32 CamFrame::getPlane() const
    {
        return m_plane;
    }

    U32 CamFrame::getBufSize() const
    {
        return m_bufSize;
    }

    U32 CamFrame::getBufId() const
    {
        return m_bufId;
    }
}
