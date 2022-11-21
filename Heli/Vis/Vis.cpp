//
// Created by tumbar on 11/20/22.
//

#include <Heli/Vis/Vis.hpp>

#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <iterator>

namespace Heli
{
    Vis::Vis(const char* componentName)
            : VisComponentBase(componentName)
    {

    }

    void Vis::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        VisComponentBase::init(queueDepth, instance);
    }

    void Vis::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        CamFrame leftFrame, rightFrame;
        frameGet_out(0, frameId, leftFrame, rightFrame);

        // Build an OpenCV Matrix with the userland memory mapped pointer
        // This memory is MemMapped to the DMA buffer
        cv::Mat left((I32) leftFrame.getInfo().height,
                     (I32) leftFrame.getInfo().width,
                     CV_8U,
                     leftFrame.getData(),
                     leftFrame.getInfo().stride);

        cv::Mat right((I32) rightFrame.getInfo().height,
                      (I32) rightFrame.getInfo().width,
                      CV_8U,
                      rightFrame.getData(),
                      rightFrame.getInfo().stride);
    }

    void Vis::CLEAR_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        m_stages.clear();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::RECTIFY_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg &calibration_file)
    {
        cv::FileStorage fs;
        if (fs.open(calibration_file.toChar(), cv::ACCESS_READ))
        {
            log_ACTIVITY_LO_CalibrationFileOpened(calibration_file);

            cv::Mat left_x, left_y, right_x, right_y;
            fs["leftMapX"] >> left_x;
            fs["leftMapY"] >> left_y;
            fs["rightMapX"] >> right_x;
            fs["rightMapY"] >> right_y;

            m_stages.emplace_back(new RectifyStage(
                    std::move(left_x), std::move(left_y),
                    std::move(right_x), std::move(right_y)));
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        }
        else
        {
            log_WARNING_LO_CalibrationFileFailed(calibration_file);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        }
    }

    void
    Vis::STEREO_cmdHandler(U32 opCode, U32 cmdSeq, Heli::Vis_StereoAlgorithm algorithm)
    {
        m_stages.emplace_back(new StereoStage(this, algorithm));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::COLORMAP_cmdHandler(U32 opCode, U32 cmdSeq, Vis_ColorMap colormap, CamSelect select)
    {
        m_stages.emplace_back(new ColormapStage(colormap, select));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
