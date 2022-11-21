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

namespace Heli
{
    class Vis;
    class VisStage
    {
    public:
        virtual void process(cv::Mat& left, cv::Mat& right) = 0;
        virtual ~VisStage() = default;
    };

    class RectifyStage : public VisStage
    {
    public:
        explicit RectifyStage(cv::Mat&& left_x,
                              cv::Mat&& left_y,
                              cv::Mat&& right_x,
                              cv::Mat&& right_y);
        void process(cv::Mat& left, cv::Mat& right) override;
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
        StereoStage(Vis* vis, const Heli::Vis_StereoAlgorithm& algorithm);

        void process(cv::Mat &left, cv::Mat &right) override;
    private:
        cv::Ptr<cv::StereoMatcher> m_stereo;
        cv::Mat m_disparity;
    };

    class ColormapStage : public VisStage
    {
    public:
        ColormapStage(const Vis_ColorMap& colormap, const CamSelect& select);

        void process(cv::Mat &left, cv::Mat &right) override;
    private:
        Vis_ColorMap m_colormap;
        CamSelect m_select;
    };
}

#endif //STEREO_HELI_VISSTAGE_HPP
