
#include "VideoStreamer.hpp"
#include "encoder/h264_encoder.hpp"
#include "output/net_output.hpp"
#include "Logger.hpp"
#include <preview/preview.hpp>
#include <functional>

#include <opencv2/opencv.hpp>

namespace Heli
{
    VideoStreamer::VideoStreamer(const char* compName)
            : VideoStreamerComponentBase(compName),
              is_showing(false),
              is_capturing(false),
              m_displaying(VideoStreamer_DisplayLocation::NONE),
              m_preview(nullptr),
              m_encoder(nullptr),
              m_net(nullptr),
              tlm_total_frames(0),
              m_eye(CamSelect::LEFT)
    {
    }

    VideoStreamer::~VideoStreamer()
    {
        delete m_preview;
        delete m_encoder;
        delete m_net;
    }

    void VideoStreamer::init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance)
    {
        VideoStreamerComponentBase::init(queueDepth, instance);

        try
        {
            m_preview = make_drm_preview();
        }
        catch (const std::runtime_error &e)
        {
            Fw::Logger::logMsg("Failed initialize preview: %s\n", (POINTER_CAST) e.what());
        }
    }

    void VideoStreamer::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        CamFrame left, right;
        bool frameValid = frameGet_out(0, frameId, left, right);

        CamFrame &frame = (m_eye == CamSelect::LEFT) ? left : right;

        if (!frameValid)
        {
            log_WARNING_LO_InvalidFrameBuffer(frameId);
            return;
        }

        tlm_total_frames++;
        tlmWrite_FramesTotal(tlm_total_frames);

        // Calculate frame rate
        Fw::Time current_time = getTime();
        Fw::Time delta = Fw::Time::sub(current_time, m_last_frame);

        F64 period = delta.getSeconds() + 1e-6 * delta.getUSeconds();
        tlmWrite_FramesPerSecond(static_cast<U32>(1.0 / period));
        m_last_frame = current_time;

        if (is_capturing)
        {
            std::string filename = m_capture.location.toChar();
            std::string extension;

            switch (m_capture.encoding.e)
            {
                case VideoStreamer_ImageEncoding::JPEG:
                    extension = ".jpg";
                    break;
                case VideoStreamer_ImageEncoding::PNG:
                    extension = ".png";
                    break;
                case VideoStreamer_ImageEncoding::TIFF:
                    extension = ".tiff";
                    break;
            }

            switch (m_capture.eye.e)
            {
                case CamSelect::LEFT:
                {
                    cv::Mat image((I32) left.getInfo().height,
                                  (I32) left.getInfo().width,
                                  CV_8U,
                                  left.getData(),
                                  left.getInfo().stride);

                    log_ACTIVITY_LO_CaptureCompleted(CamSelect::LEFT, (filename + extension).c_str());
                    cv::imwrite(filename + extension, image);
                }
                    break;
                case CamSelect::RIGHT:
                {
                    cv::Mat image((I32) right.getInfo().height,
                                  (I32) right.getInfo().width,
                                  CV_8U,
                                  right.getData(),
                                  right.getInfo().stride);

                    log_ACTIVITY_LO_CaptureCompleted(CamSelect::RIGHT, (filename + extension).c_str());
                    cv::imwrite(filename + extension, image);
                }
                    break;
                case CamSelect::BOTH:
                {
                    cv::Mat image_l((I32) left.getInfo().height,
                                    (I32) left.getInfo().width,
                                    CV_8U,
                                    left.getData(),
                                    left.getInfo().stride);
                    cv::Mat image_r((I32) right.getInfo().height,
                                    (I32) right.getInfo().width,
                                    CV_8U,
                                    right.getData(),
                                    right.getInfo().stride);

                    if (m_capture.encoding == VideoStreamer_ImageEncoding::TIFF)
                    {
                        // Store both images in a single tiff file
                        std::vector<cv::Mat> layers;
                        layers.push_back(image_l);
                        layers.push_back(image_r);

                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::BOTH, (filename + extension).c_str());
                        cv::imwrite(filename + extension, layers);
                    }
                    else
                    {
                        // Store both images in separate files
                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::LEFT, (filename + "-left" + extension).c_str());
                        cv::imwrite(filename + "-left" + extension, image_l);

                        log_ACTIVITY_LO_CaptureCompleted(CamSelect::RIGHT, (filename + "-right" + extension).c_str());
                        cv::imwrite(filename + "-right" + extension, image_r);
                    }
                }
                    break;
            }

            is_capturing = false;
            cmdResponse_out(m_capture.opCode, m_capture.cmdSeq, Fw::CmdResponse::OK);
        }

//        I32 fd = frame->buffer->planes()[0].fd.get();
        if ((m_displaying == VideoStreamer_DisplayLocation::BOTH ||
             m_displaying == VideoStreamer_DisplayLocation::UDP) && m_net)
        {
            if (!m_encoder)
            {
                m_encoder = new H264Encoder(frame.getInfo());
                m_encoder->SetInputDoneCallback([this](void* mem)
                                                {
                                                    if (encoding_buffers.empty())
                                                    {
                                                        // The video stream was swapped during stream
                                                        return;
                                                    }

                                                    // Drop the reference to the oldest frame buffer we sent to the encoder
                                                    // This assumed that the H264 encoding will reply with in order frames...
                                                    incdec_out(0, encoding_buffers.front(),
                                                               ReferenceCounter::DECREMENT);
                                                    encoding_buffers.pop();
                                                });

                m_encoder->SetOutputReadyCallback(std::bind(&Output::OutputReady, m_net,
                                                            std::placeholders::_1,
                                                            std::placeholders::_2,
                                                            std::placeholders::_3,
                                                            std::placeholders::_4));
            }

            // Once the encoder is finished it will send the data to the network
            // This will get written out as a UDP stream
            incdec_out(0, frameId, ReferenceCounter::INCREMENT);
            encoding_buffers.push(frame.getBufId());
            m_encoder->EncodeBuffer(frame.getPlane(),
                                    frame.getBufSize(),
                                    frame.getData(),
                                    frame.getInfo(),
                                    (I64) frame.getTimestamp());
//                                    (I64)frame->buffer->metadata().timestamp / 1000);
        }

        if ((m_displaying == VideoStreamer_DisplayLocation::BOTH ||
             m_displaying == VideoStreamer_DisplayLocation::HDMI) && m_preview)
        {
            incdec_out(0, frameId, ReferenceCounter::INCREMENT);
            m_preview->Show(frame);

            if (is_showing)
            {
                incdec_out(0, m_showing.getBufId(), ReferenceCounter::DECREMENT);
            }

            is_showing = true;
            m_showing = frame;
        }

        incdec_out(0, frameId, ReferenceCounter::DECREMENT);
    }

    void
    VideoStreamer::NETWORK_SEND_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg &address, U16 portN)
    {
        // Don't delete this until nobody needs it anymore
        Output* old = m_net;

        m_net = new NetOutput(address.toChar(), portN);
        if (m_encoder)
        {
            // Set up an existing encoder to write to a different net
            m_encoder->SetOutputReadyCallback(std::bind(&Output::OutputReady, m_net,
                                                        std::placeholders::_1,
                                                        std::placeholders::_2,
                                                        std::placeholders::_3,
                                                        std::placeholders::_4));
        }
        else
        {
            // Even though we need the encoder for video, we can't create it
            // yet because we don't have the stream information, we can wait
            // for the first frame the come in before we set it up
        }

        // The encoder is no longer referencing the old instance
        delete old;

        clean();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void VideoStreamer::preamble()
    {
        ActiveComponentBase::preamble();
        clean();
        m_last_frame = getTime();
    }

    void
    VideoStreamer::DISPLAY_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                      VideoStreamer_DisplayLocation where,
                                      CamSelect eye)
    {
        if (eye == CamSelect::BOTH)
        {
            log_WARNING_LO_EyeNotSupport();
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
            return;
        }

        clean();
        m_displaying = where;
        m_eye = eye;
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void VideoStreamer::clean()
    {
        if (is_showing)
        {
            incdec_out(0, m_showing.getBufId(), ReferenceCounter::DECREMENT);
            m_showing = {};
            is_showing = false;
        }

        while (!encoding_buffers.empty())
        {
            incdec_out(0, encoding_buffers.front(), ReferenceCounter::DECREMENT);
            encoding_buffers.pop();
        }
    }

    void
    VideoStreamer::CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                      const Fw::CmdStringArg &location, Heli::CamSelect eye,
                                      Heli::VideoStreamer_ImageEncoding encoding)
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

    void VideoStreamer::sched_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context)
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
