#ifndef HELI_MSP_MESSAGE_HPP
#define HELI_MSP_MESSAGE_HPP

#include <FpConfig.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Serializable.hpp>
#include <Fw/Buffer/Buffer.hpp>

#include <Heli/Fc/Fc_MspPacketTypeEnumAc.hpp>

#include <FcCfg.hpp>
#include "Heli/Fc/Fc_MspMessageIdEnumAc.hpp"
#include "Assert.hpp"

namespace Types
{
    class CircularBuffer;
}

namespace Heli
{
#define PACKED(name, contents) struct name {contents} __attribute__((packed))


    class MspMessage : public Fw::Serializable
    {
    public:
        enum
        {
            SERIALIZED_SIZE = MAX_PAYLOAD_SIZE + sizeof(U16) + sizeof(U16)  // size of buffer + storage of size word
        };

        MspMessage();
        explicit MspMessage(const Heli::Fc_MspMessageId& function);
        MspMessage(const MspMessage &other);
        ~MspMessage();

        MspMessage &operator=(const MspMessage &other);

        U32 get_size() const;
        void to_buffer(Fw::Buffer& out_buf) const;

        Fw::SerializeStatus serialize(Fw::SerializeBufferBase& buffer) const override;
        Fw::SerializeStatus deserialize(Fw::SerializeBufferBase& buffer) override;

        void set_function(const Heli::Fc_MspMessageId& function);
        Heli::Fc_MspMessageId get_function() const;

        void set_flags(U8 flags);
        U8 get_flags() const;

        Fc_MspPacketType get_direction() const;
        void set_direction(const Fc_MspPacketType& direction);

        void set_payload(const U8* payload, U16 payload_size);
        void payload_from(Types::CircularBuffer& buff, U16 payload_size);
        U16 get_payload_size() const;
        const U8* get_payload() const;
        U8 get_checksum();

        U8 v1_crc() const;
        U8 v2_crc() const;

        // Extract message data from the payload
        template <typename T>
        const T* payload(I16 size_expect = -1) const
        {
            if (size_expect >= 0)
            {
                FW_ASSERT(size_expect == get_payload_size(), size_expect, get_payload_size());
            }

            return reinterpret_cast<const T*>(&m_message[MSP_HEADER_SIZE]);
        }

    PROTECTED:
        void recompute();

        template <typename T>
        T read(U32 offset) const
        {
            T p;
            read(offset, p);
            return p;
        }

        void read(U32 offset, U8& value) const;
        void write(U32 offset, U8 element);
        void read(U32 offset, I8& value) const;
        void write(U32 offset, I8 element);

        void read(U32 offset, U16& value) const;
        void write(U32 offset, U16 element);
        void read(U32 offset, I16& value) const;
        void write(U32 offset, I16 element);

        void read(U32 offset, U32& value) const;
        void write(U32 offset, U32 element);
        void read(U32 offset, I32& value) const;
        void write(U32 offset, I32 element);

    PRIVATE:
        mutable Fw::Buffer m_buf;

        void initialize();

        // MSP packet data
        U8 m_message[MAX_PACKET_SIZE];
    };
}

#endif
