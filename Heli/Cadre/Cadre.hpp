//
// Created by tumbar on 3/31/22.
//

#ifndef HELI_CADREIMPL_H
#define HELI_CADREIMPL_H

#include <Heli/Cadre/CadreComponentAc.hpp>
#include <Heli/Cam/CamFrame.hpp>

#include <Os/Mutex.hpp>

namespace Heli
{
    class Cadre : public CadreComponentBase
    {
    public:
        explicit Cadre(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:
        void IDENTIFY_cmdHandler(U32 opCode, U32 cmdSeq, U8 agent) override;
    };
}

#endif //HELI_CADREIMPL_H
