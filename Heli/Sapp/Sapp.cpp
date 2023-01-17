//
// Created by tumbar on 1/13/23.
//

#include "Sapp.hpp"
#include "SappCfg.hpp"
#include <Eigen/Eigen>

namespace Heli
{

    Sapp::Sapp(const char* compName)
    : SappComponentBase(compName),
    m_quality(SappQuality::INVALID),
    m_position(0, 0, 0), m_attitude(0, 0, 0, 0),
    m_polling_fc(false), m_command_waiting(false), m_opCode(0), m_cmdSeq(0)
    {
    }

    void Sapp::init(NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                    NATIVE_INT_TYPE instance /*!< The instance number*/)
    {
        SappComponentBase::init(queueDepth, instance);
    }

    Quaternion Sapp::getAttitude_handler(NATIVE_INT_TYPE portNum)
    {
        // TODO(tumbar) Projection using angular velocity integral?
        m_mutex.lock();
        auto attitude = m_attitude;
        m_mutex.unlock();
        return attitude;
    }

    Vector3 Sapp::getPosition_handler(NATIVE_INT_TYPE portNum)
    {
        // TODO(tumbar) Projection using velocity integral?
        m_mutex.lock();
        auto position = m_position;
        m_mutex.unlock();
        return position;
    }

    SappQuality Sapp::getQuality_handler(NATIVE_INT_TYPE portNum)
    {
        m_mutex.lock();
        auto quality = m_quality;
        m_mutex.unlock();
        return quality;
    }

    void Sapp::schedIn_handler(NATIVE_INT_TYPE portNum,
                               NATIVE_UINT_TYPE context)
    {
        if (m_polling_fc)
        {
            // Poll the current craft position
            MspMessage msg(Fc_MspMessageId::MSP2_INAV_VISION_POSE);
            fcMsg_out(0, msg, 0, Fc_ReplyAction::REPLY);
        }

        Fw::Time current = getTime();

        if (Fw::Time::sub(current, m_last_pose_update) >=
            Fw::Time(0, SappCfg::SAPP_DEGRADE_TIMEOUT_MS * 1000))
        {
            set_quality(SappQuality::INVALID);
        }
    }

    void Sapp::fcReply_handler(NATIVE_INT_TYPE portNum,
                               const MspMessage &reply,
                               I32 ctx,
                               const Fc_ReplyStatus &status)
    {
        switch(reply.get_function().e)
        {
            case Fc_MspMessageId::MSP2_INAV_VISION_POSE:
            {
                PACKED(Pose_S,
                       F32 x;
                       F32 y;
                       F32 z;
                       F32 roll;
                       F32 pitch;
                       F32 yaw;
                       U8 vision_update;
                );

                auto pose = reply.payload<Pose_S>();

                m_position = Vector3(pose->x, pose->y, pose->z);

                Eigen::Quaternionf q;
                q = Eigen::AngleAxisf(pose->roll, Eigen::Vector3f::UnitX())
                    * Eigen::AngleAxisf(pose->pitch, Eigen::Vector3f::UnitY())
                    * Eigen::AngleAxisf(pose->yaw, Eigen::Vector3f::UnitZ());

                m_attitude = Quaternion(q.coeffs()[0], q.coeffs()[1],
                                        q.coeffs()[2], q.coeffs()[3]);

                set_quality(pose->vision_update ? SappQuality::FINE : SappQuality::DEAD_RECKON);
                m_last_pose_update = getTime();
            }
                break;

            default:
                break;
        }

        service_command_reply(reply, status);
    }

    void Sapp::service_command_reply(const MspMessage &reply,
                                     const Fc_ReplyStatus &status)
    {
        if (!m_command_waiting || reply.get_function() != m_waiting_for_reply)
        {
            return;
        }

        switch(status.e)
        {
            case Fc_ReplyStatus::OK:
                cmdResponse_out(m_opCode, m_cmdSeq, Fw::CmdResponse::OK);
                break;
            case Fc_ReplyStatus::ERROR:
                cmdResponse_out(m_opCode, m_cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
                break;
            case Fc_ReplyStatus::FULL:
            case Fc_ReplyStatus::TIMEOUT:
                cmdResponse_out(m_opCode, m_cmdSeq, Fw::CmdResponse::BUSY);
                break;
        }

        m_command_waiting = false;
        m_waiting_for_reply = 0;
        m_opCode = 0;
        m_cmdSeq = 0;
    }

    void Sapp::set_quality(const SappQuality &quality)
    {
        if (m_quality.e < quality.e)
        {
            log_ACTIVITY_HI_QualityRefined(quality);
        }
        else if (m_quality.e > quality.e)
        {
            log_ACTIVITY_HI_QualityDegraded(quality);
        }

        m_quality = quality;
    }
}
