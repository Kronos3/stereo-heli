
#include "VideoStreamer.hpp"
#include "encoder/h264_encoder.hpp"
#include "output/net_output.hpp"
#include "Logger.hpp"
#include <preview/preview.hpp>
#include <functional>

namespace Rpi
{
    VideoStreamer::VideoStreamer(const char* compName)
    : VideoStreamerComponentBase(compName),
      is_showing(false),
      m_displaying(VideoStreamer_DisplayLocation::NONE),
      m_preview(nullptr),
      m_encoder(nullptr),
      m_net(nullptr),
      m_capture{.requesting=false},
      tlm_packets_sent(0),
      tlm_total_frames(0)
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
        catch (const std::runtime_error& e)
        {
            Fw::Logger::logMsg("Failed initialize preview: %s\n", (POINTER_CAST)e.what());
        }
    }

    void VideoStreamer::frame_handler(NATIVE_INT_TYPE portNum, U32 frameId)
    {
        CamFrame frame;
        bool frameValid = frameGet_out(0, frameId, frame);
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
        tlmWrite_FramesPerSecond(1.0 / period);
        m_last_frame = current_time;

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
                    decref_out(0, encoding_buffers.front());
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
            incref_out(0, frame.getBufId());
            encoding_buffers.push(frame.getBufId());
            m_encoder->EncodeBuffer(frame.getPlane(),
                                    frame.getBufSize(),
                                    frame.getData(),
                                    frame.getInfo(),
                                    (I64)frame.getTimestamp());
//                                    (I64)frame->buffer->metadata().timestamp / 1000);
        }

        if ((m_displaying == VideoStreamer_DisplayLocation::BOTH ||
            m_displaying == VideoStreamer_DisplayLocation::HDMI) && m_preview)
        {
            incref_out(0, frame.getBufId());
            m_preview->Show(frame);

            if (is_showing)
            {
                decref_out(0, m_showing.getBufId());
            }

            is_showing = true;
            m_showing = frame;
        }

        // TODO (tumbar)
#if 0
        if (m_capture.requesting)
        {
            cv::Mat image((I32)frame->info.height,
                          (I32)frame->info.width,
                          CV_8U,
                          frame->span.data(),
                          frame->info.stride);

            bool result;
            try
            {
                result = cv::imwrite(m_capture.destination.toChar(), image);
            }
            catch (const cv::Exception& ex)
            {
                Fw::LogStringArg error_str(ex.what());
                log_WARNING_HI_ImageCaptureEncodeFailed(error_str);
                result = false;
            }

            Fw::LogStringArg destination_log(m_capture.destination);
            if (result)
            {
                log_ACTIVITY_HI_ImageCaptured(destination_log);
            }
            else
            {
                log_WARNING_HI_ImageCaptureSaveFailed(destination_log);
            }

            // Reply to the command
            cmdResponse_out(m_capture.opcode, m_capture.cmdSeq,
                            result ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);

            // Clear the capture request
            m_capture.requesting = false;
        }
#endif

        decref_out(0, frame.getBufId());
    }

    void VideoStreamer::OPEN_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg& address, U16 portN)
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
            // for the first from the come in before we set it up
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
    VideoStreamer::DISPLAY_cmdHandler(U32 opCode, U32 cmdSeq, VideoStreamer_DisplayLocation where)
    {
        clean();
        m_displaying = where;
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void VideoStreamer::clean()
    {
        if (is_showing)
        {
            decref_out(0, m_showing.getBufId());
            m_showing = {};
            is_showing = false;
        }

        while (!encoding_buffers.empty())
        {
            decref_out(0, encoding_buffers.front());
            encoding_buffers.pop();
        }
    }
}
