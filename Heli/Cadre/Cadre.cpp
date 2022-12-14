//
// Created by tumbar on 3/31/22.
//

#include <Heli/Cadre/Cadre.hpp>
#include <Fw/Types/Assert.hpp>

namespace Heli
{

    Cadre::Cadre(const char* compName)
    : CadreComponentBase(compName)
    {
    }

    void Cadre::init(NATIVE_INT_TYPE instance)
    {
        CadreComponentBase::init(instance);
    }

    void Cadre::IDENTIFY_cmdHandler(U32 opCode, U32 cmdSeq, U8 agent)
    {
        Fw::ComBuffer buf;
        I32 packet_id = 77;
        buf.serialize(packet_id);
        buf.serialize(agent);

        send_out(0, buf, 0);

        log_ACTIVITY_LO_AgentIdentify(agent);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
