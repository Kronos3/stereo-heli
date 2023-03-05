//
// Created by tumbar on 11/20/22.
//

#include <Heli/Vis/Vis.hpp>
#include <opencv2/imgcodecs.hpp>

#include <vector>

namespace Heli
{
    Vis::Vis(const char* componentName)
            : VisComponentBase(componentName), is_capturing(false)
    {
    }

    void Vis::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        VisComponentBase::init(queueDepth, instance);
    }

    void Vis::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        CamFrame leftFrame, rightFrame;
        if (!frameGet_out(0, frameId, leftFrame, rightFrame))
        {
            // Invalid frame buffer
            // Drop it
            frameOut_out(0, frameId);
            return;
        }

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

        for (auto &stage: m_stages)
        {
            // Process is performed in-place
            stage->process(left, right);
        }

        if (is_capturing)
        {
            std::string filename = m_capture.location.toChar();
            std::string extension;

            switch (m_capture.encoding.e)
            {
                case ImageEncoding::JPEG:
                    extension = ".jpg";
                    break;
                case ImageEncoding::PNG:
                    extension = ".png";
                    break;
                case ImageEncoding::TIFF:
                    extension = ".tiff";
                    break;
            }

            switch (m_capture.eye.e)
            {
                case CamSelect::LEFT:
                {
                    log_ACTIVITY_LO_CaptureCompleted(CamSelect::LEFT, (filename + extension).c_str());
                    cv::imwrite(filename + extension, left);
                }
                    break;
                case CamSelect::RIGHT:
                {
                    log_ACTIVITY_LO_CaptureCompleted(CamSelect::RIGHT, (filename + extension).c_str());
                    cv::imwrite(filename + extension, right);
                }
                    break;
                case CamSelect::BOTH:
                {
                    if (m_capture.encoding == ImageEncoding::TIFF)
                    {
                        // Store both images in a single tiff file
                        std::vector<cv::Mat> layers;
                        layers.push_back(left);
                        layers.push_back(right);

                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::BOTH, (filename + extension).c_str());
                        cv::imwrite(filename + extension, layers);
                    }
                    else
                    {
                        // Store both images in separate files
                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::LEFT, (filename + "-left" + extension).c_str());
                        cv::imwrite(filename + "-left" + extension, left);

                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::RIGHT, (filename + "-right" + extension).c_str());
                        cv::imwrite(filename + "-right" + extension, right);
                    }
                }
                    break;
            }

            is_capturing = false;
            cmdResponse_out(m_capture.opCode, m_capture.cmdSeq, Fw::CmdResponse::OK);
        }

        frameOut_out(0, frameId);
    }

    void Vis::CLEAR_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        m_stages.clear();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::RECTIFY_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        m_stages.emplace_back(new RectifyStage(m_calib));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
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

    void Vis::DEPTH_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        Fw::ParamValid valid;
        I32 pix = paramGet_DEPTH_LEFT_MASK_PIX(valid);

        m_stages.emplace_back(new DepthStage(m_calib, pix));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::SCALE_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, Heli::Vis_Interpolation interp)
    {
        m_stages.emplace_back(new ScaleStage(fx, fy, interp));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_L_K_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, F32 cx, F32 cy)
    {
        m_calib.left.k = (cv::Mat_<double>(3, 3)
                << fx, 0, cx,
                0, fy, cy,
                0, 0, 1);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_R_K_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, F32 cx, F32 cy)
    {
        m_calib.right.k = (cv::Mat_<F64>(3, 3)
                << fx, 0, cx,
                0, fy, cy,
                0, 0, 1);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_L_D_cmdHandler(U32 opCode, U32 cmdSeq, F32 a, F32 b, F32 c, F32 d, F32 e)
    {
        m_calib.left.d = (cv::Mat_<F64>(1, 5) << a, b, c, d, e);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_R_D_cmdHandler(U32 opCode, U32 cmdSeq, F32 a, F32 b, F32 c, F32 d, F32 e)
    {
        m_calib.right.d = (cv::Mat_<F64>(1, 5) << a, b, c, d, e);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_R_cmdHandler(U32 opCode, U32 cmdSeq, F32 rx, F32 ry, F32 rz)
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

        m_calib.r = R_z.mul(R_y.mul(R_x));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_T_cmdHandler(U32 opCode, U32 cmdSeq, F32 tx, F32 ty, F32 tz)
    {
        m_calib.t = (cv::Mat_<F64>(3, 1) << tx, ty, tz);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::MODEL_SIZE_cmdHandler(U32 opCode, U32 cmdSeq, U32 width, U32 height)
    {
        m_calib.size = cv::Size(static_cast<I32>(width), static_cast<I32>(height));
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Vis::CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                 const Fw::CmdStringArg &location, Heli::CamSelect eye,
                                 Heli::ImageEncoding encoding)
    {
        if (is_capturing)
        {
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::BUSY);
            return;
        }

        m_capture.request_time = getTime();

        m_capture.opCode = opCode;
        m_capture.cmdSeq = cmdSeq;

        m_capture.location = location;
        m_capture.eye = eye;
        m_capture.encoding = encoding;

        is_capturing = true;
    }

    void Vis::sched_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context)
    {
        if (is_capturing)
        {
            Fw::Time cur_time = getTime();
            Fw::Time difference = Fw::Time::sub(
                    cur_time,
                    m_capture.request_time);

            // TODO(tumbar) Do we want variable timeout?
            if (difference >= Fw::Time(cur_time.getTimeBase(), 3, 0))
            {
                is_capturing = false;
                log_WARNING_LO_CaptureTimeout(m_capture.location);
                cmdResponse_out(m_capture.opCode, m_capture.cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            }
        }
    }
}
