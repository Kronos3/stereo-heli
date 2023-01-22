//
// Created by tumbar on 1/9/23.
//

#include <Heli/Fc/Fc.hpp>

namespace Heli
{
    Fc::ReplyAwaiter::ReplyAwaiter()
    : inUse(false), opcode(static_cast<Fc_MspMessageId::t>(0)), port(0), ctx(0), action(Fc_ReplyAction::REPLY)
    {
    }

    void Fc::ReplyAwaiter::reset()
    {
        inUse = false;
        opcode = Fc_MspMessageId::MSP_MSG_NONE;
        ctx = 0;
        action = Fc_ReplyAction::REPLY;
    }

    Fc::ReplyAwaiter::ReplyAwaiter(const Fc_MspMessageId& opcode, I32 port, I32 ctx, const Fc_ReplyAction &action)
            : inUse(true), port(port), opcode(opcode), ctx(ctx), action(action)
    {

    }

    Fc::QueueItem::QueueItem(const MspMessage &msg, I32 port, I32 ctx, const Fc_ReplyAction &reply)
            : msg(msg), port(port), ctx(ctx), reply(reply)
    {
    }

    Fc::QueueItem::QueueItem()
            : ctx(0), port(0)
    {
    }

    void Fc::msgIn_handler(NATIVE_INT_TYPE portNum, MspMessage &msg, I32 ctx, const Fc_ReplyAction &reply)
    {
        queue_message(msg, portNum, ctx, reply);
    }

    void Fc::mspRecv_internalInterfaceHandler(I32 serialChannel, const Heli::MspMessage &msg)
    {
        // Check whether we are currently waiting for a reply on this channel
        if (!m_awaiting[serialChannel].inUse)
        {
            m_awaiting[serialChannel].reset();
            log_WARNING_LO_UnexpectedMspReply(serialChannel, msg.get_function());

            // Don't reply to anyone
            return;
        }

        // Make sure this is a message response or error
        if (msg.get_direction() == Fc_MspPacketType::REQUEST)
        {
            log_WARNING_LO_MspRequest(serialChannel, msg.get_function());
            return;
        }

        // Validate the function is the correct one
        if (msg.get_function() != m_awaiting[serialChannel].opcode)
        {
            log_WARNING_LO_MspUnmatchedFunctionReply(
                    serialChannel,
                    msg.get_function(),
                    m_awaiting[serialChannel].opcode);

            // We got on unexpected reply on this line
            // Ignore it
            return;
        }

        if (msg.get_direction() == Fc_MspPacketType::ERROR)
        {
            log_WARNING_LO_MspErrorReply(serialChannel, msg.get_function());

            // Reply with error
            reply(m_awaiting[serialChannel], msg, Fc_ReplyStatus::ERROR);
            return;
        }

        // Reply with success
        FW_ASSERT(msg.get_direction() == Fc_MspPacketType::RESPONSE, msg.get_direction().e);
        reply(m_awaiting[serialChannel], msg, Fc_ReplyStatus::OK);
    }

    void Fc::send_message(const MspMessage &msg, I32 port, I32 ctx, const Fc_ReplyAction &reply)
    {
        // Build the reply handler
        ReplyAwaiter handler(msg.get_function(), port, ctx, reply);

        // Check if there is a free serial line to send a message on
        m_await_mut.lock();

        bool message_sent = false;
        for (I32 i = 0; i < NUM_SERIAL_LINES; i++)
        {
            auto &bucket = m_awaiting[i];
            if (lineEnabled[i] && !bucket.inUse)
            {
                bucket = handler;
                bucket.inUse = true;

                // Mark send time of packet
                // Used for timeout detection
                bucket.timestamp = getTime();

                // Send the message out on the correct serial line
                serialSend_out(i, msg.get_buffer());

                message_sent = true;
                break;
            }
        }

        m_await_mut.unlock();

        // Make sure there was a free serial line
        FW_ASSERT(message_sent, FcComponentBase::NUM_SERIALSEND_OUTPUT_PORTS);
    }

    Fc_ReplyStatus Fc::queue_message(const MspMessage &msg, I32 port, I32 ctx, const Fc_ReplyAction &reply)
    {
        m_send_mut.lock();
        if (has_open_lines())
        {
            // The queue should be exhausted if there is a free line
            FW_ASSERT(m_queue.empty());

            // There is at least one free serial line
            // We can immediately send the message
            send_message(msg, port, ctx, reply);
            m_send_mut.unlock();
            return Fc_ReplyStatus::OK;
        }

        m_send_mut.unlock();

        // Place the item on the queue
        if (m_queue.full())
        {
            log_WARNING_LO_MessageQueueFull(FcCfg::QUEUE_MSG_LENGTH, msg.get_function());
            return Fc_ReplyStatus::FULL;
        }

        m_queue.emplace(msg, port, ctx, reply);
        return Fc_ReplyStatus::OK;
    }

    void Fc::reply(ReplyAwaiter &awaiter, const MspMessage &reply, const Fc_ReplyStatus &status)
    {
        // Only reply if the sender want it
        if (awaiter.action == Fc_ReplyAction::REPLY)
        {
            if (awaiter.port >= 0 && isConnected_msgReply_OutputPort(awaiter.port))
            {
                // An external component sent out this request
                // Reply to that component
                msgReply_out(awaiter.port, reply, awaiter.ctx, status);
            }
            else
            {
                // We (Fc) make the request
                // Reply via the internal interface
                mspReply_internalInterfaceInvoke(reply, awaiter.ctx, status);
            }
        }

        // Make the slot available again
        m_await_mut.lock();
        awaiter.reset();
        m_await_mut.unlock();

        ping_queue();
    }

    bool Fc::has_open_lines()
    {
        m_await_mut.lock();
        for (I32 i = 0; i < NUM_SERIAL_LINES; i++)
        {
            const auto &iter = m_awaiting[i];
            if (lineEnabled[i] && !iter.inUse)
            {
                m_await_mut.unlock();
                return true;
            }
        }
        m_await_mut.unlock();

        return false;
    }

    void Fc::schedIn_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context)
    {
        // Check if any of the awaiting messages have timed out
        Fw::Time current_time = getTime();

        for (I32 i = 0; i < NUM_SERIAL_LINES; i++)
        {
            auto& iter = m_awaiting[i];
            if (iter.inUse && Fw::Time::sub(current_time, iter.timestamp).getSeconds() >= MSP_TIMEOUT_S)
            {
                log_WARNING_LO_MspMessageTimeout(i, iter.opcode);
                reply(iter, MspMessage(iter.opcode), Fc_ReplyStatus::TIMEOUT);
            }
        }
    }
}