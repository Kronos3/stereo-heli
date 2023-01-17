//
// Created by tumbar on 11/20/22.
//

#include <Heli/Vis/Vis.hpp>
#include <opencv2/imgcodecs.hpp>

#include <vector>

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

        for (auto& stage : m_stages)
        {
            // Process is performed in-place
            stage->process(left, right);
        }

        frameOut_out(0, frameId);
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
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        }
    }

    void Vis::STEREO_cmdHandler(U32 opCode, U32 cmdSeq, Heli::Vis_StereoAlgorithm algorithm)
    {
        m_stages.emplace_back(new StereoStage(this, algorithm));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::COLORMAP_cmdHandler(U32 opCode, U32 cmdSeq, Vis_ColorMap colormap, CamSelect select)
    {
        m_stages.emplace_back(new ColormapStage(colormap, select));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::DEPTH_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg &calibration_file, bool is_rectified)
    {
        cv::FileStorage fs;
        if (fs.open(calibration_file.toChar(), cv::ACCESS_READ))
        {
            log_ACTIVITY_LO_CalibrationFileOpened(calibration_file);

            cv::Mat left_k, right_k;
            fs["leftK"] >> left_k;
            fs["rightK"] >> right_k;

            m_stages.emplace_back(new DepthStage(
                    this,
                    std::move(left_k), std::move(right_k),
                    is_rectified));
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        }
        else
        {
            log_WARNING_LO_CalibrationFileFailed(calibration_file);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        }
    }

    void Vis::SCALE_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, Heli::Vis_Interpolation interp)
    {
        m_stages.emplace_back(new ScaleStage(fx, fy, interp));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
