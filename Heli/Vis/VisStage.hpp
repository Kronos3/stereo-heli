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
#include <opencv2/imgproc.hpp>

#include <Heli/parallel/parallel.hpp>

namespace Heli
{
    // Parallelization params
    enum ParallelThreadInfo
    {
        T_LEFT = 0,
        T_RIGHT = 1,
        T_N
    };

    class Vis;

    struct Calibration {
        struct Intrinsic {
            cv::Mat k;  //!< Camera matrix
            cv::Mat d;  //!< Distortion parameters
        };

        Intrinsic left;
        Intrinsic right;

        bool isValid() const {
            return size.height > 0 && size.width > 0;
        }

        cv::Size size;  //!< Image frame dimensions
    };

    class VisStage
    {
    public:
        virtual void process(cv::Mat &left, cv::Mat &right) = 0;

        virtual ~VisStage() = default;
    };

    class ScaleStage : public VisStage
    {
    public:
        ScaleStage(F32 x_scale, F32 y_scale, const Vis_Interpolation& interp);
        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        libparallel::Parallelize<T_N, cv::Mat&> m_proc;

        F32 m_fx;
        F32 m_fy;
        cv::InterpolationFlags m_interp;
    };

    class RectifyStage : public VisStage
    {
    public:
        explicit RectifyStage(const Calibration& calibration);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        libparallel::Parallelize<T_N, std::tuple<cv::Mat&, cv::Mat&, cv::Mat&>> m_proc;

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
    public:
        explicit DepthStage(const Calibration& calibration, F32 baseline, U32 left_mask_pix);

        void process(cv::Mat &left, cv::Mat &right) override;

    private:
        F32 m_fx;    // x focal length in pixels
        F32 m_b;     // baseline in world coord units (cm)

        I32 m_left_mask_pix; // number of pixels to mask out
    };
}

#endif //STEREO_HELI_VISSTAGE_HPP
