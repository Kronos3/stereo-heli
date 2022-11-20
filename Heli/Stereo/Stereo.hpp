//
// Created by tumbar on 11/20/22.
//

#ifndef STEREO_HELI_STEREO_HPP
#define STEREO_HELI_STEREO_HPP

#include <Heli/Stereo/StereoComponentAc.hpp>

namespace Heli
{
    class Stereo : public StereoComponentBase
    {
    public:
        explicit Stereo(const char* componentName);

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

    PRIVATE:
        void frame_handler(
                NATIVE_INT_TYPE portNum, /*!< The port number*/
                U32 frameId
        ) override;
    };
}

#endif //STEREO_HELI_STEREO_HPP
