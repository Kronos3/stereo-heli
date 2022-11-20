//
// Created by tumbar on 11/20/22.
//

#include <Heli/Stereo/Stereo.hpp>

namespace Heli
{
    Stereo::Stereo(const char *componentName)
    : StereoComponentBase(componentName)
    {

    }

    void Stereo::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        StereoComponentBase::init(queueDepth, instance);
    }

    void Stereo::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        CamFrame left, right;
        frameGet_out(0, frameId, left, right);
    }
}
