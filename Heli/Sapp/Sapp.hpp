//
// Created by tumbar on 3/31/22.
//

#ifndef STEREO_HELI_SAPP_HPP
#define STEREO_HELI_SAPP_HPP

#include <Heli/Sapp/SappComponentAc.hpp>

namespace Heli
{
    class Sapp : public SappComponentBase
    {
    public:
        explicit Sapp(const char* compName);

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
                );

    PRIVATE:
        void schedIn_handler(NATIVE_INT_TYPE portNum,
                             NATIVE_UINT_TYPE context) override;

        void fcReply_handler(NATIVE_INT_TYPE portNum,
                             const MspMessage &reply,
                             I32 ctx,
                             const Fc_ReplyStatus &status) override;

        Quaternion getAttitude_handler(NATIVE_INT_TYPE portNum) override;
        Vector3 getPosition_handler(NATIVE_INT_TYPE portNum) override;
        SappQuality getQuality_handler(NATIVE_INT_TYPE portNum) override;

        void START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
        void STOP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
        void SET_ATTITUDE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 roll_deg, F32 pitch_deg, F32 yaw_deg) override;
        void SET_POSITION_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 x, F32 y, F32 z) override;

    PRIVATE:
        void service_command_reply(const MspMessage &reply, const Fc_ReplyStatus &status);
        void set_quality(const SappQuality& quality);

    PRIVATE:
        Fw::Time m_last_pose_update;
        SappQuality m_quality;
        Quaternion m_attitude;
        Vector3 m_position;

        Os::Mutex m_mutex;

        bool m_polling_fc;
        bool m_command_waiting;
        Fc_MspMessageId m_waiting_for_reply;
        FwOpcodeType m_opCode;
        U32 m_cmdSeq;
    };
}

#endif //STEREO_HELI_SAPP_HPP
