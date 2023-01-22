//
// Created by tumbar on 1/2/23.
//

#include <Heli/Fc/Fc.hpp>

namespace Heli
{

    void Fc::serialRecv_handler(NATIVE_INT_TYPE portNum,
                                Fw::Buffer &serBuffer,
                                Drv::SerialReadStatus &status)
    {
        allocate_for(portNum);

        if (status != Drv::SerialReadStatus::SER_OK)
        {
            if (m_state == Fc_State::OK
                || m_state == Fc_State::NOT_CONNECTED)
            {
                // State will swap to bad state
                // Send out a warning
                log_WARNING_HI_SerialRecvError(status);
                set_state(Fc_State::BAD_SERIAL);
            }

            // Data is bad, exit early
            // Don't queue it to the ring buffer
        }
        else if (serBuffer.getSize() > 0)
        {
            m_tlm_BytesRecv += serBuffer.getSize();

            // Data is good, queue it to the ring buffer
            m_buffer[portNum].serialize(serBuffer.getData(), serBuffer.getSize());

            // Ping our main thread to let them know new data is ready
            data_ready(portNum);
        }

        // Return the read buffer to the uart driver
        deallocate_out(0, serBuffer);
    }

#define READ(v, o) do { \
    if (m_buffer[serialChannel].peek((v), (o)) != Fw::SerializeStatus::FW_SERIALIZE_OK) \
        return;\
} while (0)

    void Fc::data_ready(I32 serialChannel)
    {
        while(true)
        {
            // Rotate the data to the front of a packet
            U32 off = 0;
            U8 b;
            Fw::SerializeStatus stat;
            while(true)
            {
                if ((stat = m_buffer[serialChannel].peek(b, off)) != Fw::SerializeStatus::FW_SERIALIZE_OK
                    || b == '$')
                {
                    break;
                }

                off++;
            }

            // Rotate out the data before a packet is found
            if (off)
            {
                log_WARNING_LO_DumpedExtraneousBytes(off, serialChannel);
            }

            m_buffer[serialChannel].rotate(off);
            if (stat != Fw::SerializeStatus::FW_SERIALIZE_OK)
            {
                // We are out of data
                return;
            }

            process_message(serialChannel);
        }
    }

    void Fc::process_message(I32 serialChannel)
    {
        U8 b;

        // Read out the MSP message
        READ(b, 0);
        FW_ASSERT(b == '$', b);

        // Check what type of packet this is
        // If it's not valid rotate out the bytes and exit out
        bool is_v2;
        READ(b, 1);
        switch(b)
        {
            case 'M':
                is_v2 = false;
                break;
            case 'X':
                is_v2 = true;
                break;
            default:
                log_WARNING_LO_PacketError(Fc_PacketError::MAGIC, b);
                m_buffer[serialChannel].rotate(2);
                return;
        }

        U8 direction;
        READ(direction, 2);
        switch(direction)
        {
            case Fc_MspPacketType::RESPONSE:
            case Fc_MspPacketType::REQUEST:
            case Fc_MspPacketType::ERROR:
                break;
            default:
                log_WARNING_LO_PacketError(Fc_PacketError::DIRECTION, direction);
                m_buffer[serialChannel].rotate(3);
                return;
        }

        MspMessage msg;
        msg.set_direction(static_cast<Fc_MspPacketType::t>(direction));

        if (is_v2)
        {
            U8 crc, flag;
            U16 function, payload_size;
            READ(flag, 3);
            READ(function, 4);
            READ(payload_size, 6);
            READ(crc, payload_size + 8);

            m_buffer[serialChannel].rotate(8);

            if (payload_size > MAX_PAYLOAD_SIZE)
            {
                log_WARNING_LO_PayloadTooLarge(payload_size, MAX_PACKET_SIZE);
                log_WARNING_LO_PacketError(Fc_PacketError::PAYLOAD_SIZE, '!');
                return;
            }

            msg.set_function(static_cast<Fc_MspMessageId::t>(function));
            msg.payload_from(m_buffer[serialChannel], payload_size);
            m_buffer[serialChannel].rotate(payload_size + 1);

            if (crc != msg.v2_crc())
            {
                log_WARNING_LO_PacketError(Fc_PacketError::CHECKSUM, crc);
                return;
            }
        }
        else
        {
            U8 crc, payload_size, function;
            READ(payload_size, 3);
            READ(function, 4);
            READ(crc, payload_size + 5);

            m_buffer[serialChannel].rotate(5);

            msg.set_function(static_cast<Fc_MspMessageId::t>(function));
            msg.payload_from(m_buffer[serialChannel], payload_size);
            m_buffer[serialChannel].rotate(payload_size + 1);

            if (crc != msg.v1_crc())
            {
                log_WARNING_LO_PacketError(Fc_PacketError::CHECKSUM, crc);
                return;
            }
        }

        mspRecv_internalInterfaceInvoke(serialChannel, msg);
    }
}