//
// Created by tumbar on 12/29/22.
//

#include "MspCircularBuffer.hpp"

namespace Msp
{
    CircularBuffer::CircularBuffer()
    : Types::CircularBuffer(m_ring, sizeof(m_ring)), m_ring{}
    {
    }

    Fw::SerializeStatus CircularBuffer::peek(U16 &value, NATIVE_UINT_TYPE offset) const
    {
        // Deserialize all the bytes from Msp format (little-endian)
        value = 0;
        U8 b;
        for (NATIVE_UINT_TYPE i = 0; i < sizeof(U16); i++) {
            auto stat = Types::CircularBuffer::peek(b, offset + i);
            if (stat != Fw::SerializeStatus::FW_SERIALIZE_OK)
            {
                return stat;
            }

            // Load little endian
            value |= (b << (i * 8));
        }

        return Fw::FW_SERIALIZE_OK;
    }

    Fw::SerializeStatus CircularBuffer::peek(U32 &value, NATIVE_UINT_TYPE offset) const
    {
        auto status = Types::CircularBuffer::peek(value, offset);
        if (status != Fw::SerializeStatus::FW_SERIALIZE_OK)
        {
            return status;
        }

        // Swap big to little endian
        value = (value & 0x0000FFFF) << 16 | (value & 0xFFFF0000) >> 16;
        value = (value & 0x00FF00FF) << 8 | (value & 0xFF00FF00) >> 8;

        return Fw::FW_SERIALIZE_OK;
    }

    Fw::SerializeStatus CircularBuffer::peek(char &value, NATIVE_UINT_TYPE offset) const
    {
        return Types::CircularBuffer::peek(value, offset);
    }

    Fw::SerializeStatus CircularBuffer::peek(U8 &value, NATIVE_UINT_TYPE offset) const
    {
        return Types::CircularBuffer::peek(value, offset);
    }

    Fw::SerializeStatus CircularBuffer::peek(U8* buffer, NATIVE_UINT_TYPE size, NATIVE_UINT_TYPE offset) const
    {
        return Types::CircularBuffer::peek(buffer, size, offset);
    }
}
