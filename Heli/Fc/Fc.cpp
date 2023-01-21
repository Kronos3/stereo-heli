//
// Created by tumbar on 12/28/22.
//

#include <Heli/Fc/Fc.hpp>

namespace Heli
{
    Fc::Fc(const char* componentName)
            : FcComponentBase(componentName),
              m_buffer{},
              m_cmdSeq(0), m_opcode(0), m_command_awaiting(false),
              m_state(Fc_State::NOT_CONNECTED),
              lineEnabled{}
    {
        memset(&lineEnabled, true, sizeof(lineEnabled));
    }

    void Fc::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        FcComponentBase::init(queueDepth, instance);
    }

    void Fc::allocate(NATIVE_UINT_TYPE number)
    {
        Fw::Buffer buff;
        // request a buffer and pass it on to the UART for each requested
        for (NATIVE_UINT_TYPE buffNum = 0; buffNum < number; buffNum++)
        {
            for (NATIVE_UINT_TYPE lineNum = 0; lineNum < NUM_SERIAL_LINES; lineNum++)
            {
                buff = allocate_out(0, MAX_PACKET_SIZE);
                FW_ASSERT(buff.getSize() >= MAX_PACKET_SIZE, buff.getSize(), MAX_PACKET_SIZE);
                FW_ASSERT(buff.getData());
                readBufferSend_out(lineNum, buff);
            }
        }
    }


    void Fc::preamble()
    {
        allocate(FcCfg::FC_NUM_BUFFERS);
        reset();
    }

    void Fc::reset()
    {
        log_ACTIVITY_HI_Reset();

        // Reinitialize the Fc
        // We will attempt to ping the control to get some status
        // and identifier information out of it
        set_state(Fc_State::NOT_CONNECTED);
        queue_message(
                MspMessage(Fc_MspMessageId::MSP_API_VERSION),
                -1, 0, Fc_ReplyAction::REPLY);
    }

    // Make sure hold is little endian (life is simpler this way)
#if defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN)
    static_assert(__BYTE_ORDER == __LITTLE_ENDIAN);
#else
    // You're on your own here
    // Just make sure you are compiling to a LE target
#endif

    void Fc::mspReply_internalInterfaceHandler(const MspMessage &reply, I32 ctx, const Fc_ReplyStatus &status)
    {
        if (status != Fc_ReplyStatus::OK)
        {
            log_WARNING_LO_ErrorDuringIdentification(reply.get_function());
            set_state(Fc_State::BAD_FIRMWARE_IDENT);
            return;
        }

        switch (reply.get_function().e)
        {
            case Fc_MspMessageId::MSP_API_VERSION:
            {
                auto response = reply.payload<Metadata::ApiVersion>();

                // Our modified iNAV API is version 2.5+
                if (response->version_major != 2 ||
                    response->version_minor < 5)
                {
                    log_WARNING_HI_UnsupportedApiVersion(
                            response->version_major,
                            response->version_minor);
                    set_state(Fc_State::BAD_API_VERSION);
                }
                else
                {
                    m_metadata.api_version = *response;

                    // Ask for the next piece of information
                    queue_message(
                            MspMessage(Fc_MspMessageId::MSP_FC_VARIANT),
                            -1, ctx, Fc_ReplyAction::REPLY);
                }
            }
            case Fc_MspMessageId::MSP_FC_VARIANT:
            {
                auto response = reply.payload<char[4]>();

                // Our modified iNAV API is version 2.5+
                if (strncmp(*response, "INAV", 4) != 0)
                {
                    log_WARNING_HI_UnsupportedIdentifier(*response);
                    set_state(Fc_State::BAD_FIRMWARE_IDENT);
                }
                else
                {
                    strncpy(m_metadata.fc_variant, *response, 4);

                    // Ask for the next piece of information
                    queue_message(
                            MspMessage(Fc_MspMessageId::MSP_BOARD_INFO),
                            -1, ctx, Fc_ReplyAction::REPLY);
                }
            }
            case Fc_MspMessageId::MSP_BOARD_INFO:
            {
                auto response = reply.payload<Metadata::FcBoardInfo>(false);
                m_metadata.board_info = *response;

                log_ACTIVITY_HI_ConnectionEstablished(m_metadata.api_version.version_major,
                                                      m_metadata.api_version.version_minor,
                                                      m_metadata.fc_variant,
                                                      m_metadata.board_info.board_ident);
                set_state(Fc_State::OK);
            }
                break;
            default:
                break;
        }
    }

    void Fc::RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
    {
        m_opcode = opCode;
        m_cmdSeq = cmdSeq;
        m_command_awaiting = true;
        reset();
    }

    void Fc::set_state(const Fc_State &state)
    {
        if (state == Fc_State::NOT_CONNECTED)
        {
            // This is not a command finisher state
            m_state = Fc_State::NOT_CONNECTED;
            return;
        }

        log_ACTIVITY_LO_FcStateChange(state);
        m_state = state;

        if (m_command_awaiting)
        {
            cmdResponse_out(m_opcode, m_cmdSeq,
                            (m_state == Fc_State::OK)
                                ? Fw::CmdResponse::OK
                                : Fw::CmdResponse::EXECUTION_ERROR);
        }

        m_command_awaiting = false;
        m_opcode = 0;
        m_cmdSeq = 0;
    }

    void Fc::DISABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 uart)
    {
        if (uart >= NUM_SERIAL_LINES)
        {
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
            return;
        }

        log_ACTIVITY_HI_DisableUart(uart);

        m_send_mut.lock();
        lineEnabled[uart] = false;
        m_send_mut.unlock();

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Fc::ENABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 uart)
    {
        if (uart >= NUM_SERIAL_LINES)
        {
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
            return;
        }

        log_ACTIVITY_HI_EnableUart(uart);

        m_send_mut.lock();
        lineEnabled[uart] = true;
        m_send_mut.unlock();

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Fc::GET_LINES_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
    {
        m_send_mut.lock();
        for (I32 i = 0; i < NUM_SERIAL_LINES; i++)
        {
            if (lineEnabled[i])
            {
                log_ACTIVITY_HI_EnableUart(i);
            }
            else
            {
                log_ACTIVITY_HI_DisableUart(i);
            }
        }
        m_send_mut.unlock();

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
