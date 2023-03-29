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

        Transform getFrame_handler(NATIVE_INT_TYPE portNum,
                                   const Fm_Frame &f,
                                   const Fm_Frame &respective) override;

        Fm_Frame getParent_handler(NATIVE_INT_TYPE portNum, const Fm_Frame &f) override;

        void transform_handler(NATIVE_INT_TYPE portNum, const Fm_Frame &f,
                               const Heli::Transform &delta) override;

        void GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Fm_Frame f, Fm_Frame p) override;

        void SET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Fm_Frame f, Fm_Frame p,
                            Fm_Relationship relation, F32 tx, F32 ty, F32 tz, F32 rx, F32 ry, F32 rz) override;

        struct TreeNode
        {
            Transform T;
            Fm_Frame parent;
            Fm_Relationship relation;
        };

    PRIVATE:

        Fm_Frame get_common_parent(
                const Fm_Frame &a,
                const Fm_Frame &b) const;

        TreeNode m_tree[Fm_Frame::NUM_CONSTANTS];
    };
}

#endif //STEREO_HELI_FM_HPP
