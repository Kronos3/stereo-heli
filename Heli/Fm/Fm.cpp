//
// Created by tumbar on 3/13/23.
//

#include "Fm.hpp"

namespace Heli
{

    Fm::Fm(const char* compName) : FmComponentBase(compName)
    {
    }

    void Fm::init(NATIVE_INT_TYPE instance)
    {
        FmComponentBase::init(instance);
    }

    template<typename T, typename R = cv::Mat3f>
    static inline R get_rotation_matrix(T rx, T ry, T rz)
    {
        // Convert from euler angles back to rotation matrix
        R R_x = (R(3, 3)
                << 1, 0, 0,
                0, cos(rx), -sin(rx),
                0, sin(rx), cos(rx));

        R R_y = (R(3, 3)
                << cos(ry), 0, sin(ry),
                0, 1, 0,
                -sin(ry), 0, cos(ry));

        R R_z = (R(3, 3)
                << cos(rz), -sin(rz), 0,
                sin(rz), cos(rz), 0,
                0, 0, 1);

        return R_z.mul(R_y.mul(R_x));
    }

    static inline
    void get_euler_angles(const cv::Mat &R, F32 &rx, F32 &ry, F32 &rz)
    {
        float sy = sqrtf(R.at<F32>(0, 0) * R.at<F32>(0, 0) + R.at<F32>(1, 0) * R.at<F32>(1, 0));

        bool singular = sy < 1e-6; // If

        if (!singular)
        {
            rx = atan2f(R.at<F32>(2, 1), R.at<F32>(2, 2));
            ry = atan2f(-R.at<F32>(2, 0), sy);
            rz = atan2f(R.at<F32>(1, 0), R.at<F32>(0, 0));
        }
        else
        {
            rx = atan2f(-R.at<F32>(1, 2), R.at<F32>(1, 1));
            ry = atan2f(-R.at<F32>(2, 0), sy);
            rz = 0;
        }
    }

    Fm_Frame Fm::get_common_parent(
            const Fm_Frame &a,
            const Fm_Frame &b) const
    {
        if (a == Fm_Frame::NONE || b == Fm_Frame::NONE)
        {
            return Fm_Frame::NONE;
        }

        if (a == b)
        {
            return a;
        }

        auto pa = get_common_parent(m_tree[a.e].parent, b);
        if (pa != Fm_Frame::NONE)
        {
            return pa;
        }

        auto pb = get_common_parent(a, m_tree[b.e].parent);
        if (pa != Fm_Frame::NONE)
        {
            return pb;
        }

        return Fm_Frame::NONE;
    }

    Heli::Transform
    Fm::getFrame_handler(NATIVE_INT_TYPE portNum, const Fm_Frame &f, const Fm_Frame &r)
    {
        if (f == r)
        {
            // Identity
            return Transform();
        }

        Fm_Frame p = get_common_parent(f, r);
        if (p == Fm_Frame::NONE)
        {
            log_WARNING_HI_NoCommonParent(p, f);
            return Transform(false);
        }

        Transform rTp;
        auto rTp_f = r;
        while (rTp_f != p)
        {
            rTp = rTp * m_tree[p.e].T.inverse();
            rTp_f = m_tree[p.e].parent;
        }

        Transform fTp;
        auto fTp_f = f;
        while (fTp_f != p)
        {
            fTp = fTp * m_tree[p.e].T.inverse();
            fTp_f = m_tree[p.e].parent;
        }

        return rTp * fTp.inverse();
    }

    Fm_Frame Fm::getParent_handler(NATIVE_INT_TYPE portNum, const Fm_Frame &f)
    {
        return m_tree[f.e].parent;
    }

    void Fm::GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Fm_Frame f, Fm_Frame p)
    {
        auto tform = getFrame_handler(0, f, p);
        if (!tform.is_valid())
        {
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        F32 rx, ry, rz;
        get_euler_angles(tform.R(), rx, ry, rz);

        const auto& t = tform.t();

        log_ACTIVITY_HI_TransformR(p, f, rx, ry, rz);
        log_ACTIVITY_HI_TransformT(p, f, t(0), t(1), t(2));

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Fm::SET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                            Fm_Frame f,
                            Fm_Frame p,
                            Fm_Relationship relation,
                            F32 tx, F32 ty, F32 tz, F32 rx, F32 ry, F32 rz)
    {
        m_tree[f.e].parent = p;
        m_tree[f.e].relation = relation;
        m_tree[f.e].T = Transform(
                get_rotation_matrix(rx, ry, rz),
                {tx, ty, tz});

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
