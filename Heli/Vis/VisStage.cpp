//
// Created by tumbar on 11/20/22.
//

#include <Heli/Vis/Vis.hpp>
#include <Heli/Vis/VisStage.hpp>

#include <utility>
#include "Assert.hpp"
#include "opencv2/imgproc.hpp"

namespace Heli
{

    ScaleStage::ScaleStage(F32 x_scale, F32 y_scale, Vis_Interpolation interp)
            : m_fx(x_scale), m_fy(y_scale)
    {
        switch(interp.e)
        {
            case Vis_Interpolation::NEAREST:
                m_interp = cv::INTER_NEAREST;
                break;
            case Vis_Interpolation::LINEAR:
                m_interp = cv::INTER_LINEAR;
                break;
            case Vis_Interpolation::CUBIC:
                m_interp = cv::INTER_CUBIC;
                break;
        }
    }

    void ScaleStage::process(cv::Mat &left, cv::Mat &right)
    {
        cv::resize(left, left,
                   cv::Size(0, 0),
                   m_fx, m_fy,
                   m_interp);
        cv::resize(right, right,
                   cv::Size(0, 0),
                   m_fx, m_fy,
                   m_interp);
    }

    RectifyStage::RectifyStage(
            cv::Mat&& left_x,
            cv::Mat&& left_y,
            cv::Mat&& right_x,
            cv::Mat&& right_y)
    : m_left_x(std::move(left_x)),
    m_left_y(std::move(left_y)),
    m_right_x(std::move(right_x)),
    m_right_y(std::move(right_y))
    {
    }

    void RectifyStage::process(cv::Mat &left, cv::Mat &right)
    {
        cv::remap(left, left, m_left_x, m_left_y, cv::INTER_LINEAR);
        cv::remap(right, right, m_right_x, m_right_y, cv::INTER_LINEAR);
    }

    StereoStage::StereoStage(
            Vis* vis, const Vis_StereoAlgorithm& algorithm)
    {
        Fw::ParamValid valid;
        I32 preFilterCap = vis->paramGet_STEREO_PRE_FILTER_CAP(valid);
        I32 uniquenessRatio = vis->paramGet_STEREO_UNIQUENESS_RATIO(valid);

        switch(algorithm.e)
        {
            case Vis_StereoAlgorithm::BLOCK_MATCHING:
            {
                auto stereo_bm = cv::StereoBM::create();
                I32 textureThreshold = vis->paramGet_STEREO_BM_TEXTURE_THRESHOLD(valid);

                stereo_bm->setPreFilterCap(preFilterCap);
                stereo_bm->setUniquenessRatio(uniquenessRatio);
                stereo_bm->setTextureThreshold(textureThreshold);
                m_stereo = stereo_bm;
            }
                break;
            case Vis_StereoAlgorithm::SEMI_GLOBAL_BLOCK_MATCHING:
            {
                auto stereo_sgbm = cv::StereoSGBM::create();
                stereo_sgbm->setPreFilterCap(preFilterCap);
                stereo_sgbm->setUniquenessRatio(uniquenessRatio);
                m_stereo = stereo_sgbm;
            }
                break;
        }

        I32 blockSize = vis->paramGet_STEREO_BLOCK_SIZE(valid);
        I32 minDisparity = vis->paramGet_STEREO_MIN_DISPARITY(valid);
        I32 numDisparity = vis->paramGet_STEREO_NUM_DISPARITIES(valid);
        I32 speckleWindowSize = vis->paramGet_STEREO_SPECKLE_WINDOW_SIZE(valid);
        I32 speckleRange = vis->paramGet_STEREO_SPECKLE_RANGE(valid);

        m_stereo->setBlockSize(blockSize);
        m_stereo->setMinDisparity(minDisparity);
        m_stereo->setNumDisparities(numDisparity);
        m_stereo->setSpeckleWindowSize(speckleWindowSize);
        m_stereo->setSpeckleRange(speckleRange);
        m_stereo->setDisp12MaxDiff(1);
    }

    void StereoStage::process(cv::Mat &left, cv::Mat &right)
    {
        m_stereo->compute(left, right, m_disparity);
        m_disparity.convertTo(right, CV_8U);
    }

    ColormapStage::ColormapStage(const Vis_ColorMap &colormap, const CamSelect &select)
    : m_colormap(colormap), m_select(select)
    {
    }

    void ColormapStage::process(cv::Mat &left, cv::Mat &right)
    {
        if (m_select == CamSelect::LEFT || m_select == CamSelect::BOTH)
        {
            cv::applyColorMap(left, left, m_colormap.e);
        }

        if (m_select == CamSelect::RIGHT || m_select == CamSelect::BOTH)
        {
            cv::applyColorMap(right, right, m_colormap.e);
        }
    }

    DepthStage::DepthStage(
            Vis* vis,
            cv::Mat &&proj_left,
            cv::Mat &&proj_right,
            bool is_rectified)
    : m_is_rectified(is_rectified),
      m_left(std::move(proj_left)), m_right(std::move(proj_right))
    {
        // Get focal length of x axis for left camera
        m_f = m_left.k.at<F64>(0, 0);

        // Calculate baseline of stereo pair
        if (m_is_rectified)
        {
            m_b = m_right.t.at<double>(0) - m_left.t.at<double>(0);
        }
        else
        {
            m_b = m_left.t.at<double>(0) - m_right.t.at<double>(0);
        }

        Fw::ParamValid valid;
        m_left_mask_pix = vis->paramGet_DEPTH_LEFT_MASK_PIX(valid);
    }

    void DepthStage::process(cv::Mat &left, cv::Mat &right)
    {
        cv::Mat& disp = right;

        // Avoid instability and division by zero
        disp.setTo(0.1, disp == 0);
        disp.setTo(0.1, disp == -1.0);

        // Make empty depth map then fill with depth
        disp = m_f * m_b / disp;

        // Mask out the left side of the image
        // This is caused by the right camera not seeing this portion of the image
        cv::Mat mask = cv::Mat::zeros(disp.size[0], disp.size[1], CV_8U);
        I32 ymax = disp.size[0];
        I32 xmax = disp.size[1];

        cv::rectangle(
                mask,
                cv::Point(m_left_mask_pix, 0),
                cv::Point(xmax, ymax),
                255, -1);

        disp.setTo(0, mask);
    }

    DepthStage::CameraCalibration::CameraCalibration(cv::Mat &&projection)
    {
        cv::decomposeProjectionMatrix(projection, k, r, t);
    }
}
