#ifndef HELI_CAMFRAME_HPP
#define HELI_CAMFRAME_HPP

#include <Heli/Cam/CamFrameBaseSerializableAc.hpp>
#include "core/stream_info.hpp"

namespace Rpi
{
    class CamFrame : private CamFrameBase
    {
    public:
        enum {
            SERIALIZED_SIZE = CamFrameBase::SERIALIZED_SIZE
        };

        CamFrame();
        CamFrame(U32 bufId, U8* data,
                 U32 bufSize,
                 U32 width, U32 height,
                 U32 stride, U64 timestamp, I32 plane);
        CamFrame(const CamFrameBase& src);

        U8* getData() const;
        U32 getBufSize() const;
        U32 getBufId() const;
        StreamInfo getInfo() const;
        U64 getTimestamp() const;
        I32 getPlane() const;
    };
}

#endif //HELI_CAMFRAME_HPP
