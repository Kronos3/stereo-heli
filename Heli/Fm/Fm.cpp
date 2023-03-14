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

    static inline
    cv::Mat get_rotation_matrix(F64 rx, F64 ry, F64 rz)
    {

        // Convert from euler angles back to rotation matrix
        cv::Mat R_x = (cv::Mat_<F64>(3, 3)
                << 1, 0, 0,
                0, cos(rx), -sin(rx),
                0, sin(rx), cos(rx));

        cv::Mat R_y = (cv::Mat_<F64>(3, 3)
                << cos(ry), 0, sin(ry),
                0, 1, 0,
                -sin(ry), 0, cos(ry));

        cv::Mat R_z = (cv::Mat_<F64>(3, 3)
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

    CoordinateFrame Fm::get_common_parent(
            const CoordinateFrame &a,
            const CoordinateFrame &b) const
    {
        if (a == CoordinateFrame::NONE || b == CoordinateFrame::NONE)
        {
            return CoordinateFrame::NONE;
        }

        if (a == b)
        {
            return a;
        }

        auto pa = get_common_parent(m_parent[a.e], b);
        if (pa != CoordinateFrame::NONE)
        {
            return pa;
        }

        auto pb = get_common_parent(a, m_parent[b.e]);
        if (pa != CoordinateFrame::NONE)
        {
            return pb;
        }

        return CoordinateFrame::NONE;
    }

    Heli::Transform
    Fm::getFrame_handler(NATIVE_INT_TYPE portNum, const CoordinateFrame &f, const CoordinateFrame &r)
    {
        if (f == r)
        {
            // Identity
            return Transform();
        }

        CoordinateFrame p = get_common_parent(f, r);
        if (p == CoordinateFrame::NONE)
        {
            log_WARNING_HI_NoCommonParent(p, f);
            return Transform(false);
        }

        Transform rTp;
        auto rTp_f = r;
        while (rTp_f != p)
        {
            rTp = rTp * m_frame_tree[p.e].inverse();
            rTp_f = m_parent[p.e];
        }

        Transform fTp;
        auto fTp_f = f;
        while (fTp_f != p)
        {
            fTp = fTp * m_frame_tree[p.e].inverse();
            fTp_f = m_parent[p.e];
        }

        return rTp * fTp.inverse();
    }

    Heli::CoordinateFrame Fm::getParent_handler(NATIVE_INT_TYPE portNum, const CoordinateFrame &f)
    {
        return m_parent[f.e];
    }

    void Fm::setFrame_handler(NATIVE_INT_TYPE portNum, const CoordinateFrame &f, Transform &t)
    {
        m_frame_tree[f.e] = t;
    }

    void Fm::setParent_handler(NATIVE_INT_TYPE portNum,
                               const CoordinateFrame &f,
                               const CoordinateFrame &parent,
                               Transform &t)
    {
        m_parent[f.e] = parent;
        m_frame_tree[f.e] = t;
    }

    void Fm::GET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Heli::CoordinateFrame f, Heli::CoordinateFrame p)
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
                            Heli::CoordinateFrame f,
                            Heli::CoordinateFrame p,
                            F32 tx, F32 ty, F32 tz, F32 rx, F32 ry, F32 rz)
    {
        m_parent[f.e] = p;

        cv::Mat r = get_rotation_matrix(rx, ry, rz);
        cv::Mat t({3, 1}, {tx, ty, tz});

        m_frame_tree[f.e] = Transform(r, t);

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
