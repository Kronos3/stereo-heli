#ifndef HELI_CAMBUFFER_HPP
#define HELI_CAMBUFFER_HPP

#include "fprime/Fw/Types/Serializable.hpp"
#include "libcamera/libcamera/framebuffer.h"
#include "Rpi/Cam/core/stream_info.hpp"
#include "Rpi/Cam/core/completed_request.hpp"

#include <functional>
#include <atomic>

namespace Rpi
{
    class CamBuffer
    {
    public:
        CamBuffer();
        CamBuffer(U32 id_);

        U32 id;

        CompletedRequest* request;
        StreamInfo info;
        libcamera::FrameBuffer* buffer;
        libcamera::Span<U8> span;

        void incref();
        void decref();
        bool in_use() const;

        void register_callback(std::function<void(CompletedRequest*)> return_cb);

    private:
        std::function<void(CompletedRequest*)> return_buffer;
        std::atomic<I32> ref_count;

        void clear();
    };
}

#endif //HELI_CAMBUFFER_HPP
