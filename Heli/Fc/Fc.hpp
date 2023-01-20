//
// Created by tumbar on 12/28/22.
//

#ifndef STEREO_HELI_FC_HPP
#define STEREO_HELI_FC_HPP

#include <Heli/Fc/FcComponentAc.hpp>
#include <Heli/Types/MspCircularBuffer.hpp>

#include <Os/Mutex.hpp>

#include "MspMessage.hpp"
#include "Heli/Types/TQueue.hpp"

namespace Heli
{
    class Fc : public FcComponentBase
    {
    public:
        enum
        {
            NUM_SERIAL_LINES = FcComponentBase::NUM_SERIALSEND_OUTPUT_PORTS
        };

        explicit Fc(const char* componentName);

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

        //! Allocate pool of buffers for UART receive - BufferManager and UART
        //  instances must be connected and ready. BufferManager should have at least
        //  number+1 buffers allocated
        void allocate(NATIVE_UINT_TYPE number);

    PRIVATE:
        void serialRecv_handler(NATIVE_INT_TYPE portNum, Fw::Buffer &serBuffer, Drv::SerialReadStatus &status) override;
        void mspRecv_internalInterfaceHandler(I32 serialChannel, const Heli::MspMessage& msg) override;
        void data_ready(I32 serialChannel);

        void schedIn_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context) override;

        void RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

        void msgIn_handler(NATIVE_INT_TYPE portNum, Heli::MspMessage &msg, I32 ctx, const Heli::Fc_ReplyAction &reply) override;
        void mspReply_internalInterfaceHandler(const Heli::MspMessage &reply, I32 ctx, const Heli::Fc_ReplyStatus &status) override;

        Fc_ReplyStatus queue_message(const MspMessage& msg, I32 port, I32 ctx, const Heli::Fc_ReplyAction &reply);
        void send_message(const MspMessage& msg, I32 port, I32 ctx, const Heli::Fc_ReplyAction &reply);
        void process_message(I32 serialChannel);

    PRIVATE:
        struct ReplyAwaiter {
            bool inUse;
            Fc_MspMessageId opcode;
            I32 port;
            I32 ctx;
            Fc_ReplyAction action;

            Fw::Time timestamp;

            ReplyAwaiter();
            ReplyAwaiter(const Fc_MspMessageId& opcode, I32 port, I32 ctx, const Fc_ReplyAction& action);
            void reset();
        };

        struct QueueItem
        {
            MspMessage msg;
            I32 port;
            I32 ctx;
            Fc_ReplyAction reply;

            QueueItem();
            QueueItem(const MspMessage &msg, I32 port, I32 ctx, const Fc_ReplyAction &reply);
        };

        void reply(ReplyAwaiter& awaiter, const MspMessage& reply, const Fc_ReplyStatus& status);
        bool has_open_lines();

    PRIVATE:

        struct Metadata
        {
            PACKED(ApiVersion,
                   U8 protocol_version;
                   U8 version_major;
                   U8 version_minor;
            ) api_version;
            char fc_variant[4]{};
            PACKED(FcBoardInfo,
                   char board_ident[4];
                   U16 hardware_revision;
                   U8 osd_support;
                   U8 common_capabilities;
                   U8 target_name_length;
            ) board_info;
        } m_metadata;

        void preamble() override;
        void reset();

        // State machine reply handlers
        void sm_not_connected(const MspMessage &reply, I32 ctx, const Fc_ReplyStatus &status);
        void sm_ok(const MspMessage &reply, I32 ctx, const Fc_ReplyStatus &status);

        bool m_command_awaiting;
        FwOpcodeType m_opcode;
        U32 m_cmdSeq;

        void set_state(const Fc_State& state);

    PRIVATE:

        // Messages awaiting to receive replies
        // Able to await messages from multiple serial lines at once
        Os::Mutex m_await_mut;
        ReplyAwaiter m_awaiting[NUM_SERIAL_LINES];

        Fc_State m_state;

        Os::Mutex m_send_mut;
        Msp::CircularBuffer m_buffer[NUM_SERIAL_LINES];
        Types::TQueue<QueueItem, FcCfg::QUEUE_MSG_LENGTH> m_queue;
    };
}

#endif //STEREO_HELI_FC_HPP
