#ifndef HELI_CAMBUFFER_HPP
#define HELI_CAMBUFFER_HPP

#include "fprime/Fw/Types/Serializable.hpp"
#include "Heli/Cam/core/stream_info.hpp"
#include "Heli/Cam/core/completed_request.hpp"

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

        CompletedRequest* left_request;
        CompletedRequest* right_request;

        StreamInfo info;
        libcamera::FrameBuffer* left_fb;
        libcamera::Span<U8> left_span;
        libcamera::FrameBuffer* right_fb;
        libcamera::Span<U8> right_span;

        void incref();
        void decref();
        bool in_use() const;

        void register_callback(std::function<void(CompletedRequest*, CompletedRequest*)> return_cb);

    private:
        std::function<void(CompletedRequest*, CompletedRequest*)> return_buffer;
        std::atomic<I32> ref_count;

        void clear();
    };
}

#endif //HELI_CAMBUFFER_HPP
