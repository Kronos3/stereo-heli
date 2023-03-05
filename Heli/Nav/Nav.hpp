//
// Created by tumbar on 12/26/22.
//

#ifndef STEREO_HELI_NAV_HPP
#define STEREO_HELI_NAV_HPP

#include <Heli/Nav/NavComponentAc.hpp>

namespace Heli
{
    class Nav : public NavComponentBase
    {
    public:
        explicit Nav(const char* compName);
        ~Nav() override;

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

    PRIVATE:

        void frame_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;
        void STOP_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void TRACK_cmdHandler(U32 opCode, U32 cmdSeq) override;

    PRIVATE:
    };
}

#endif //STEREO_HELI_NAV_HPP
