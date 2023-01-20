#include <Heli/Fc/MspMessage.hpp>
#include <Fw/Types/Assert.hpp>

#include <Utils/Types/CircularBuffer.hpp>

#include "crc.h"

#include <cstring>
#include <algorithm>


namespace Heli
{
    MspMessage::MspMessage(const Heli::Fc_MspMessageId& function)
    : m_message{}
    {
        initialize();
        set_function(function);
    }

    MspMessage::MspMessage()
    : m_message{}
    {
        initialize();
    }

    MspMessage::MspMessage(const MspMessage &other)
    : m_message{}
    {
        std::copy_n(other.m_message, other.get_payload_size() + 9, m_message);
    }

    MspMessage::~MspMessage() = default;

    MspMessage &MspMessage::operator=(const MspMessage &other)
    {
        if (&other != this)
        {
            std::copy_n(other.m_message, other.get_payload_size() + 9, m_message);
        }

        return *this;
    }

    NATIVE_UINT_TYPE MspMessage::getBuffCapacity() const
    {
        return MAX_PAYLOAD_SIZE - get_payload_size();
    }

    U8* MspMessage::getBuffAddr()
    {
        return m_message;
    }

    const U8* MspMessage::getBuffAddr() const
    {
        return m_message;
    }

    Fw::SerializeStatus MspMessage::serialize(Fw::SerializeBufferBase &buffer) const
    {
        return buffer.serialize(m_message, get_payload_size() + FcCfg::MSP_OVERHEAD, false);
    }

    Fw::SerializeStatus MspMessage::deserialize(Fw::SerializeBufferBase &buffer)
    {
        NATIVE_UINT_TYPE length = get_payload_size() + FcCfg::MSP_OVERHEAD;
        return buffer.deserialize(m_message, length, false);
    }

    void MspMessage::initialize()
    {
        m_message[0] = '$';

#if STEREO_HELI_USE_MSPV2
        m_message[1] = 'X';
#else
        m_message[1] = 'M';
#error "MSPV1 not implemented"
#endif
        m_message[2] = Fc_MspPacketType::REQUEST;
        m_message[3] = 0; // flag, set to 0
        m_message[4] = 0; // function, lsb
        m_message[5] = 0; // function, msb
        m_message[6] = 0; // payload size, lsb
        m_message[7] = 0; // payload size, msb

        recompute();
    }

    Fc_MspMessageId MspMessage::get_function() const
    {
        return static_cast<Fc_MspMessageId::t>(read<U16>(4));
    }

    U16 MspMessage::get_payload_size() const
    {
        return read<U16>(6);
    }

    Fc_MspPacketType MspMessage::get_direction() const
    {
        return static_cast<Fc_MspPacketType::t>(m_message[2]);
    }

    const U8* MspMessage::get_payload() const
    {
        return m_message + 8;
    }

    void MspMessage::set_direction(const Fc_MspPacketType &type)
    {
        m_message[2] = type.e;
        recompute();
    }

    void MspMessage::set_payload(const U8* payload, U16 payload_size)
    {
        write(6, payload_size);
        memcpy(m_message + 8, payload, payload_size);
        recompute();
    }

    void MspMessage::set_function(const Heli::Fc_MspMessageId& function)
    {
        write(4, static_cast<U16>(function.e));
        recompute();
    }

    void MspMessage::payload_from(Types::CircularBuffer &buff, U16 payload_size)
    {
        write(6, payload_size);
        buff.peek(m_message + 8, payload_size);
        recompute();
    }

    U8 MspMessage::v2_crc() const
    {
        return crc8_dvb_s2_update(0, m_message + 3, get_payload_size() + 5);
    }

    U8 MspMessage::v1_crc() const
    {
        return crc8_xor_update(0, m_message + 3, get_payload_size() + 2);
    }

    void MspMessage::recompute()
    {
        // Compute the CRC
#if STEREO_HELI_USE_MSPV2
        U8 crc = v2_crc();
#else
        U8 crc = v1_crc();
#endif

        m_message[get_payload_size() + 8] = crc;
    }

    void MspMessage::read(U32 offset, U8 &value) const
    {
        value = m_message[offset];
    }

    void MspMessage::write(U32 offset, U8 element)
    {
        m_message[offset] = element;
    }

    void MspMessage::read(U32 offset, I8 &value) const
    {
        value = static_cast<I8>(m_message[offset]);
    }

    void MspMessage::write(U32 offset, I8 element)
    {
        m_message[offset] = static_cast<U8>(element);
    }

    void MspMessage::read(U32 offset, U16 &value) const
    {
        // Little endian
        value = m_message[offset] | ((U16)m_message[offset + 1] << 8);
    }

    void MspMessage::write(U32 offset, U16 element)
    {
        // Little endian
        m_message[offset] = element & 0xFF;
        m_message[offset + 1] = (element >> 8) & 0xFF;
    }

    void MspMessage::read(U32 offset, I16 &value) const
    {
        // Little endian
        value = static_cast<I16>((I16)m_message[offset] | ((I16)m_message[offset + 1] << 8));
    }

    void MspMessage::write(U32 offset, I16 element)
    {
        // Little endian
        m_message[offset] = element & 0xFF;
        m_message[offset + 1] = (element >> 8) & 0xFF;
    }

    void MspMessage::read(U32 offset, U32 &value) const
    {
        // Little endian
        value = m_message[offset]
                | (m_message[offset + 1] << 8)
                | (m_message[offset + 2] << 16)
                | (m_message[offset + 3] << 24);
    }

    void MspMessage::write(U32 offset, U32 element)
    {
        m_message[offset] = element & 0xFF;
        m_message[offset + 1] = (element >> 8) & 0xFF;
        m_message[offset + 2] = (element >> 16) & 0xFF;
        m_message[offset + 3] = (element >> 24) & 0xFF;
    }

    void MspMessage::read(U32 offset, I32 &value) const
    {
        // Little endian
        value = m_message[offset]
                | (m_message[offset + 1] << 8)
                | (m_message[offset + 2] << 16)
                | (m_message[offset + 3] << 24);
    }

    void MspMessage::write(U32 offset, I32 element)
    {
        m_message[offset] = element & 0xFF;
        m_message[offset + 1] = (element >> 8) & 0xFF;
        m_message[offset + 2] = (element >> 16) & 0xFF;
        m_message[offset + 3] = (element >> 24) & 0xFF;
    }
}
