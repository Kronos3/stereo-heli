#include "CamBuffer.hpp"

#include <utility>
#include "Assert.hpp"

namespace Rpi
{
    Rpi::CamBuffer::CamBuffer(U32 id_)
    : CamBuffer()
    {
        this->id = id_;
    }

    void CamBuffer::incref()
    {
        ref_count++;
    }

    void CamBuffer::decref()
    {
        if (ref_count == 1)
        {
            // Free the frame buffer
            clear();
        }
        else
        {
            ref_count--;
        }
    }

    void CamBuffer::clear()
    {
        FW_ASSERT(ref_count == 1, ref_count);
        FW_ASSERT(request);

        // Returns the buffer back to the camera
        return_buffer(request);

        request = nullptr;
        buffer = nullptr;
        size_t s = 0;
        span = libcamera::Span<U8>(nullptr, s);
        ref_count = 0;
    }

    bool CamBuffer::in_use() const
    {
        return ref_count > 0;
    }

    void CamBuffer::register_callback(std::function<void(CompletedRequest*)> return_cb)
    {
        return_buffer = std::move(return_cb);
    }

    CamBuffer::CamBuffer()
    : id(0), request(nullptr), buffer(nullptr), ref_count(0)
    {
    }
}
