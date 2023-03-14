//
// Created by tumbar on 3/13/23.
//

#ifndef STEREO_HELI_FM_HPP
#define STEREO_HELI_FM_HPP

#include <Heli/Fm/FmComponentAc.hpp>

namespace Heli
{
    class Fm : public FmComponentBase
    {
    public:
        explicit Fm(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:

        Heli::Transform getFrame_handler(NATIVE_INT_TYPE portNum,
                                         const Heli::CoordinateFrame &f,
                                         const Heli::CoordinateFrame &respective) override;

        Heli::CoordinateFrame getParent_handler(NATIVE_INT_TYPE portNum, const Heli::CoordinateFrame &f) override;
        void setFrame_handler(NATIVE_INT_TYPE portNum, const Heli::CoordinateFrame &f, Heli::Transform &t) override;
        void setParent_handler(NATIVE_INT_TYPE portNum,
                               const Heli::CoordinateFrame &f,
                               const Heli::CoordinateFrame &parent,
                               Heli::Transform &t) override;

        void GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Heli::CoordinateFrame f, Heli::CoordinateFrame p) override;
        void SET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Heli::CoordinateFrame f, Heli::CoordinateFrame p,
                            F32 tx, F32 ty, F32 tz, F32 rx, F32 ry, F32 rz) override;

    PRIVATE:
        CoordinateFrame get_common_parent(
                const CoordinateFrame& a,
                const CoordinateFrame& b) const;

        Transform m_frame_tree[CoordinateFrame::NUM_CONSTANTS];
        CoordinateFrame m_parent[CoordinateFrame::NUM_CONSTANTS];
    };
}

#endif //STEREO_HELI_FM_HPP
