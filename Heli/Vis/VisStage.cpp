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
        switch(algorithm.e)
        {
            case Vis_StereoAlgorithm::BLOCK_MATCHING:
            {
                auto stereo_bm = cv::StereoBM::create();

                Fw::ParamValid valid;
                I32 preFilterCap = vis->paramGet_STEREO_BM_PRE_FILTER_CAP(valid);
                I32 textureThreshold = vis->paramGet_STEREO_BM_TEXTURE_THRESHOLD(valid);
                I32 uniquenessRatio = vis->paramGet_STEREO_BM_UNIQUENESS_RATIO(valid);

                stereo_bm->setPreFilterCap(preFilterCap);
                stereo_bm->setTextureThreshold(textureThreshold);
                stereo_bm->setUniquenessRatio(uniquenessRatio);
                m_stereo = stereo_bm;
            }
                break;
            case Vis_StereoAlgorithm::SEMI_GLOBAL_BLOCK_MATCHING:
            {
                Fw::ParamValid valid;
                I32 preFilterCap = vis->paramGet_STEREO_BM_PRE_FILTER_CAP(valid);
                I32 uniquenessRatio = vis->paramGet_STEREO_BM_UNIQUENESS_RATIO(valid);

                auto stereo_sgbm = cv::StereoSGBM::create();
                stereo_sgbm->setPreFilterCap(preFilterCap);
                stereo_sgbm->setUniquenessRatio(uniquenessRatio);
                m_stereo = stereo_sgbm;
            }
                break;
        }

        Fw::ParamValid valid;
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
        m_disparity.convertTo(left, CV_8U);
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
}
