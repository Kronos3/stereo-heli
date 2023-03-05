//
// Created by tumbar on 11/20/22.
//

#include <Heli/Vis/Vis.hpp>
#include <Heli/Vis/VisStage.hpp>

#include "Assert.hpp"
#include "opencv2/imgproc.hpp"

namespace Heli
{
    ScaleStage::ScaleStage(F32 x_scale, F32 y_scale, const Vis_Interpolation& interp)
            : m_fx(x_scale), m_fy(y_scale),
              m_proc([this](cv::Mat& src_dest) {
                  cv::resize(src_dest, src_dest,
                             cv::Size(0, 0),
                             m_fx, m_fy,
                             m_interp);
              })
    {
        switch (interp.e)
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

    void ScaleStage::process(cv::Mat& left, cv::Mat& right)
    {
        auto& a1 = m_proc.feed<T_LEFT>(left);
        auto& a2 = m_proc.feed<T_RIGHT>(right);

        a1.await();
        a2.await();
    }

    RectifyStage::RectifyStage(const Calibration& calibration)
            : m_proc([](std::tuple<cv::Mat&, cv::Mat&, cv::Mat&> t) {
        cv::remap(std::get<0>(t), std::get<0>(t),
                  std::get<1>(t), std::get<2>(t),
                  cv::INTER_LINEAR);
    })
    {
        cv::initUndistortRectifyMap(calibration.left.k, calibration.left.d,
                                    cv::noArray(), cv::noArray(),
                                    calibration.size, CV_32FC1,
                                    m_left_x, m_left_y);

        cv::initUndistortRectifyMap(calibration.right.k, calibration.right.d,
                                    cv::noArray(), cv::noArray(),
                                    calibration.size, CV_32FC1,
                                    m_right_x, m_right_y);
    }

    void RectifyStage::process(cv::Mat& left, cv::Mat& right)
    {
        auto& a1 = m_proc.feed<T_LEFT>({left, m_left_x, m_left_y});
        auto& a2 = m_proc.feed<T_RIGHT>({right, m_right_x, m_right_y});

        a1.await();
        a2.await();
    }

    StereoStage::StereoStage(
            Vis* vis, const Vis_StereoAlgorithm& algorithm)
    {
        Fw::ParamValid valid;
        I32 preFilterCap = vis->paramGet_STEREO_PRE_FILTER_CAP(valid);
        I32 uniquenessRatio = vis->paramGet_STEREO_UNIQUENESS_RATIO(valid);

        switch (algorithm.e)
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

    void StereoStage::process(cv::Mat& left, cv::Mat& right)
    {
        m_stereo->compute(left, right, m_disparity);
        m_disparity.convertTo(right, CV_8U);
    }

    ColormapStage::ColormapStage(const Vis_ColorMap& colormap, const CamSelect& select)
            : m_colormap(colormap), m_select(select)
    {
    }

    void ColormapStage::process(cv::Mat& left, cv::Mat& right)
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

    DepthStage::DepthStage(const Calibration& calibration, U32 left_mask_pix)
            : m_fx(calibration.left.k.at<F64>(0, 0)),
              m_b(std::abs(calibration.t.at<F64>(0, 0))),
              m_left_mask_pix(static_cast<I32>(left_mask_pix))
    {
    }

    void DepthStage::process(cv::Mat& left, cv::Mat& right)
    {
        cv::Mat& disp = right;

        // Avoid instability and division by zero
        disp.setTo(0.1, disp == 0);
        disp.setTo(0.1, disp == -1.0);

        // Make empty depth map then fill with depth
        disp = m_fx * m_b / disp;

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
}
