#include "CamBuffer.hpp"

#include <utility>
#include "Assert.hpp"

namespace Heli
{
    CamBuffer::CamBuffer(U32 id_)
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
        FW_ASSERT(left_request);
        FW_ASSERT(right_request);

        // Returns the buffer back to the camera
        return_buffer(left_request, right_request);

        left_request = nullptr;
        right_request = nullptr;

        left_fb = nullptr;
        right_fb = nullptr;

        size_t s = 0;
        left_span = libcamera::Span<U8>(nullptr, s);
        right_span = libcamera::Span<U8>(nullptr, s);
        ref_count = 0;

        invalid = false;
    }

    bool CamBuffer::in_use() const
    {
        return ref_count > 0;
    }

    void CamBuffer::register_callback(std::function<void(CompletedRequest*, CompletedRequest*)> return_cb)
    {
        return_buffer = std::move(return_cb);
    }

    CamBuffer::CamBuffer()
    : id(0), invalid(false),
    left_request(nullptr), right_request(nullptr),
    left_fb(nullptr), right_fb(nullptr), ref_count(0)
    {
    }

    bool CamBuffer::is_invalid() const
    {
        return invalid;
    }

    void CamBuffer::invalidate()
    {
        invalid = true;
    }
}
