//
// Created by tumbar on 12/26/22.
//

#include "Nav.hpp"

namespace Heli
{

    Nav::Nav(const char* compName) : NavComponentBase(compName),
    vo(nullptr)
    {
    }

    void Nav::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        NavComponentBase::init(queueDepth, instance);
    }

    void Nav::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {


        frameOut_out(0, frameId);
    }

    void Nav::STOP_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }

    void Nav::TRACK_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }

    Nav::~Nav()
    {
        delete vo;
    }
}
