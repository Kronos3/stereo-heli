//
// Created by tumbar on 3/31/22.
//

#include <Heli/FramePipe/FramePipe.hpp>
#include <Fw/Types/Assert.hpp>

namespace Heli
{

    FramePipe::FramePipe(const char* compName)
    : FramePipeComponentBase(compName),
    m_pipeline_n(0), m_valid(false)
    {
    }

    void FramePipe::init(NATIVE_INT_TYPE instance)
    {
        FramePipeComponentBase::init(instance);
    }

    void FramePipe::send_to_idx(U32 idx, U32 frameId)
    {
        // Make sure the pipeline is valid
        if (!m_valid)
        {
            log_WARNING_LO_NotValidated();
            return;
        }

        if (idx >= m_pipeline_n)
        {
            // Finished processing
            // Give the frame back to the camera
            incdec_out(0, frameId, ReferenceCounter::DECREMENT);
            return;
        }

        auto& stage = m_pipeline[idx];
        if (stage.inStage != -1)
        {
            // Perform requested action if the pipeline stage is not
            // ready for this frame yet
            switch(stage.type.e)
            {
                case FramePipe_ComponentType::DROP_ON_FULL:
                    // Drop the frame
                    incdec_out(0, frameId, ReferenceCounter::DECREMENT);
                    return;
                case FramePipe_ComponentType::WAIT_ON_FULL_DROP:
                    if (stage.queue.full())
                    {
                        // Drop the oldest frame
                        U32 oldFrameId = stage.queue.pop();
                        incdec_out(0, oldFrameId, ReferenceCounter::DECREMENT);
                    }
                    // fallthrough
                case FramePipe_ComponentType::WAIT_ON_FULL_ASSERT:
                    stage.queue.push(frameId);
                    return;
                case FramePipe_ComponentType::NO_REPLY:
                    // We don't expect replies from here
                    // This stage will always look full
                    // Send the frame anyway
                    break;
            }
        }

        stage.inStage = static_cast<I32>(frameId);
        frame_out(stage.component.e, frameId);
    }

    void FramePipe::camFrame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        m_mutex.lock();
        send_to_idx(0, frameId);
        m_mutex.unlock();
    }

    void FramePipe::frameIn_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        // Find the correct stage this frameId comes from
        m_mutex.lock();
        I32 stageIdx = -1;
        for (I32 i = 0; i < m_pipeline_n; i++)
        {
            if (m_pipeline[i].inStage == frameId)
            {
                stageIdx = i;
                break;
            }
        }

        if (stageIdx < 0)
        {
            m_mutex.unlock();
            log_WARNING_LO_FrameNotFound(frameId, portNum);
            incdec_out(0, frameId, ReferenceCounter::DECREMENT);
            return;
        }

        FramePipe::Stage& stage = m_pipeline[stageIdx];

        // Mark this pipeline as ready
        m_pipeline->inStage = -1;

        // Send waiting frame into stage if there is one
        if (!stage.queue.empty())
        {
            send_to_idx(stageIdx, stage.queue.pop());
        }

        // Send this stage's output into the next stage's input
        send_to_idx(stageIdx + 1, frameId);

        m_mutex.unlock();
    }

    void FramePipe::CLEAR_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        m_mutex.lock();
        m_pipeline_n = 0;
        m_valid = false;
        m_mutex.unlock();

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void FramePipe::PUSH_cmdHandler(
            U32 opCode, U32 cmdSeq,
            FramePipe_Component cmp,
            FramePipe_ComponentType cmpType)
    {
        m_mutex.lock();
        if (m_pipeline_n >= FramePipe_PIPELINE_N)
        {
            m_mutex.unlock();
            log_WARNING_LO_CheckFailed(FramePipe_CheckFailure::FULL);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
            return;
        }

        // Must 'CHECK' after a stage is added
        m_valid = false;

        FramePipe::Stage& stage = m_pipeline[m_pipeline_n++];
        stage.inStage = -1;
        stage.queue.clear();
        stage.component = cmp;
        stage.type = cmpType;

        m_mutex.unlock();

        log_ACTIVITY_LO_StageAdded(cmp);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void FramePipe::CHECK_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        FW_ASSERT(m_pipeline_n <= FramePipe_PIPELINE_N, m_pipeline_n);

        m_mutex.lock();
        m_valid = false;

        if (m_pipeline_n == 0)
        {
            m_mutex.unlock();

            log_WARNING_LO_CheckFailed(FramePipe_CheckFailure::EMPTY);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
            return;
        }

        for (U32 i = 0; i < m_pipeline_n; i++)
        {
            if (m_pipeline[i].type == FramePipe_ComponentType::NO_REPLY)
            {
                if (i + 1 != m_pipeline_n)
                {
                    m_mutex.unlock();

                    log_WARNING_LO_CheckFailed(FramePipe_CheckFailure::NO_REPLY_INPUT);
                    cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
                    return;
                }
            }
        }

        m_valid = true;
        m_mutex.unlock();

        log_ACTIVITY_LO_CheckPassed();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void FramePipe::SHOW_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        std::string str;
        for (U32 i = 0; i < m_pipeline_n; i++)
        {
            Fw::String compName;
            m_pipeline[i].component.toString(compName);
            str += compName.toChar();

            if (i + 1 < m_pipeline_n)
            {
                str += " -> ";
            }
        }

        Fw::LogStringArg arg(str.c_str());
        log_ACTIVITY_LO_FramePipeline(arg);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    FramePipe::Fifo::Fifo()
    : m_buffer{0}, n(0)
    {
    }

    bool FramePipe::Fifo::empty() const
    {
        return n == 0;
    }

    bool FramePipe::Fifo::full() const
    {
        return n >= FramePipe_PIPELINE_QUEUE_N;
    }

    void FramePipe::Fifo::push(U32 frameId)
    {
        FW_ASSERT(!full(), n);
        m_buffer[n++] = frameId;
    }

    U32 FramePipe::Fifo::pop()
    {
        FW_ASSERT(!empty());
        U32 out = m_buffer[0];

        // Nice and simple
        // Not the quickest but the easiest :)
        memmove(&m_buffer[0],
                &m_buffer[1],
                sizeof(U32) * (FramePipe_PIPELINE_QUEUE_N - 1));

        return out;
    }

    void FramePipe::Fifo::clear()
    {
        memset(m_buffer, 0, sizeof(m_buffer));
        n = 0;
    }
}
