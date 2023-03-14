//
// Created by tumbar on 3/12/23.
//

#include "NavVo.hpp"

namespace Heli
{
    struct VoSystemImpl
    {
    };

    VoSystem::VoSystem()
    : impl(new VoSystemImpl)
    {
    }

    VoSystem::~VoSystem()
    {
        delete impl;
    }
}
