//
// Created by tumbar on 11/20/22.
//

#ifndef STEREO_HELI_VISSTAGE_HPP
#define STEREO_HELI_VISSTAGE_HPP

#include <Fw/Types/String.hpp>
#include <Heli/Cam/CamFrame.hpp>
#include <Heli/Vis/Vis_StereoAlgorithmEnumAc.hpp>

#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>
#include "opencv2/imgproc.hpp"

namespace Heli
{
    class Vis;

    class VisStage
    {
    public:
        virtual void process(cv::Mat &left, cv::Mat &right) = 0;

        virtual ~VisStage() = default;
    };

    class ScaleStage : public VisStage
    {
    public:
        ScaleStage(F32 x_scale, F32 y_scale, Vis_Interpolation interp);
        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        F32 m_fx;
        F32 m_fy;
        cv::InterpolationFlags m_interp;
    };

    class RectifyStage : public VisStage
    {
    public:
        explicit RectifyStage(cv::Mat &&left_x,
                              cv::Mat &&left_y,
                              cv::Mat &&right_x,
                              cv::Mat &&right_y);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        // Rectification maps to reproject epi-polar lines to be
        // parallel on both land and right images. Gets rid of distortion
        // using intrinsic parameters and epi-polar projection with extrinsic
        // calibration parameters.
        cv::Mat m_left_x;
        cv::Mat m_left_y;

        cv::Mat m_right_x;
        cv::Mat m_right_y;
    };

    class StereoStage : public VisStage
    {
    public:
        StereoStage(Vis* vis, const Heli::Vis_StereoAlgorithm &algorithm);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        cv::Ptr <cv::StereoMatcher> m_stereo;
        cv::Mat m_disparity;
    };

    class ColormapStage : public VisStage
    {
    public:
        ColormapStage(const Vis_ColorMap &colormap, const CamSelect &select);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        Vis_ColorMap m_colormap;
        CamSelect m_select;
    };

    class DepthStage : public VisStage
    {
        struct CameraCalibration
        {
            cv::Mat t;
            cv::Mat r;
            cv::Mat k;

            CameraCalibration(cv::Mat &&projection);
        };

    public:
        explicit DepthStage(Vis* vis,
                            cv::Mat &&p_left,
                            cv::Mat &&p_right,
                            bool is_rectified);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        bool m_is_rectified;
        F64 m_f;    // focal length
        F64 m_b;    // baseline

        I32 m_left_mask_pix;

        CameraCalibration m_left;
        CameraCalibration m_right;
    };
}

#endif //STEREO_HELI_VISSTAGE_HPP
