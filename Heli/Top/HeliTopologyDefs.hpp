//
// Created by tumbar on 9/21/22.
//

#ifndef HELI_RPITOPOLOGYDEFS_HPP
#define HELI_RPITOPOLOGYDEFS_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/MallocAllocator.hpp>

namespace Heli
{
    namespace Allocation {

        // Malloc allocator for topology construction
        extern Fw::MallocAllocator mallocator;

    }

    // State for topology construction
    struct TopologyState
    {
    };
}

#endif //HELI_RPITOPOLOGYDEFS_HPP
