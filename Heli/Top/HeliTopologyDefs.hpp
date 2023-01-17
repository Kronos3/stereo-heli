//
// Created by tumbar on 9/21/22.
//

#ifndef HELI_RPITOPOLOGYDEFS_HPP
#define HELI_RPITOPOLOGYDEFS_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/MallocAllocator.hpp>

#include <Os/Log.hpp>

#include <Svc/FramingProtocol/FprimeProtocol.hpp>

namespace Heli
{
    namespace Allocation {

        // Malloc allocator for topology construction
        extern Fw::MallocAllocator mallocator;

    }

    namespace Init {

        // Initialization status
        extern bool status;

    }

    // State for topology construction
    struct TopologyState
    {
        TopologyState() :
                hostName(nullptr),
                portNumber(0)
        {
        }
        TopologyState(
                const char *hostName,
                U32 portNumber
        ) :
                hostName(hostName),
                portNumber(portNumber)
        {
        }

        const char* hostName;
        U32 portNumber;
    };
}

#endif //HELI_RPITOPOLOGYDEFS_HPP
