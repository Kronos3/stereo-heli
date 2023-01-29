//
// Created by tumbar on 12/28/22.
//

#ifndef STEREO_HELI_FCCFG_HPP
#define STEREO_HELI_FCCFG_HPP

#include <Fw/Types/BasicTypes.hpp>

#ifndef STEREO_HELI_USE_MSPV2
#define STEREO_HELI_USE_MSPV2 1
#endif

namespace Heli {
    enum FcCfg {
        //!< Maximum Payload size
        //!< Even though MSPV2 allows for up to 16-bit payloads,
        //!<   inav does not support larger responses than this
        //!< Requests are limited to 192
        MAX_PAYLOAD_SIZE = 4096 + 16,
//        MAX_PAYLOAD_SIZE = 65535,
        //!< MSP header + size + checksum
#if STEREO_HELI_USE_MSPV2
        MSP_HEADER_SIZE = 8,
        MSP_CHECKSUM_SIZE = 1,
#else
        MSP_HEADER_SIZE = 4,
        MSP_CHECKSUM_SIZE = 1,
#endif
        MSP_V1_OVERHEAD = 4 + MSP_CHECKSUM_SIZE,
        MSP_OVERHEAD = MSP_HEADER_SIZE + MSP_CHECKSUM_SIZE,
        MAX_PACKET_SIZE = MSP_HEADER_SIZE + MAX_PAYLOAD_SIZE + MSP_CHECKSUM_SIZE,
        //! The size of the circular buffer in bytes
        RING_BUFFER_SIZE = MAX_PACKET_SIZE * 8,

        // We can hold up to 20 messages in the Fc msg queue
        QUEUE_MSG_LENGTH = 20,

        MSP_TIMEOUT_S = 1,

        INAV_MSP_RC_CHANNEL = 8,
        FC_NUM_BUFFERS = 8
    };
}

#endif //STEREO_HELI_FCCFG_HPP
