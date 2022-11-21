
#include "Cam.hpp"
#include <core/libcamera_app.h>

namespace Heli
{

    Cam::Cam(const char *compName)
            : CamComponentBase(compName),
              m_left(new LibcameraApp()),
              m_right(new LibcameraApp()),
              tlm_dropped(0),
              tlm_captured(0),
              m_streaming(false)
    {
    }

    void Cam::init(NATIVE_INT_TYPE instance)
    {
        CamComponentBase::init(instance);
    }

    void Cam::configure(I32 videoWidth, I32 videoHeight,
                        I32 left_id, I32 right_id,
                        I32 l_rotation, bool l_vflip, bool l_hflip,
                        I32 r_rotation, bool r_vflip, bool r_hflip)
    {
        for (auto &buffer: m_buffers)
        {
            buffer.register_callback([this](CompletedRequest *cr_l, CompletedRequest *cr_r)
                                     {  m_left->queueRequest(cr_l);
                                        m_right->queueRequest(cr_r); });
        }

        m_left->OpenCamera(left_id);
        m_right->OpenCamera(right_id);

        m_left->ConfigureCameraStream(libcamera::Size(videoWidth, videoHeight),
                                      l_rotation, l_vflip, l_hflip);
        m_right->ConfigureCameraStream(libcamera::Size(videoWidth, videoHeight),
                                       r_rotation, r_vflip, r_hflip);
    }

    Cam::~Cam()
    {
        delete m_left;
        delete m_right;
    }

    void Cam::streaming_thread()
    {
        while (true)
        {
            // Wait for both camera to respond
            LibcameraApp::Msg msg_l = m_left->Wait();
            LibcameraApp::Msg msg_r = m_right->Wait();

            // Exit the stream thread if either of the camera request quit
            if (msg_l.type == LibcameraApp::MsgType::Quit
                || msg_r.type == LibcameraApp::MsgType::Quit)
            {
                break;
            }

            FW_ASSERT(msg_l.type == LibcameraApp::MsgType::RequestComplete, (I32) msg_l.type);
            FW_ASSERT(msg_r.type == LibcameraApp::MsgType::RequestComplete, (I32) msg_r.type);

            tlm_captured++;
            tlmWrite_FramesCapture(tlm_captured);

            // Get an internal frame buffer
            CamBuffer *buffer = get_buffer();
            if (!buffer)
            {
                // Ran out of frame buffers
                tlm_dropped++;
                tlmWrite_FramesDropped(tlm_dropped);
                m_left->queueRequest(msg_l.payload);
                m_right->queueRequest(msg_r.payload);
                continue;
            }

            buffer->left_request = msg_l.payload;
            buffer->right_request = msg_l.payload;

            libcamera::Stream* left_stream = m_left->GetStream();
            libcamera::Stream* right_stream = m_right->GetStream();

            // Get the DMA buffer
            buffer->left_fb = msg_l.payload->buffers[left_stream];
            buffer->right_fb = msg_r.payload->buffers[right_stream];

            // Make sure we got DMA buffers from the request
            FW_ASSERT(buffer->left_fb);
            FW_ASSERT(buffer->right_fb);

            // Get the userland pointer
            buffer->left_span = m_left->Mmap(buffer->left_fb)[0];
            buffer->right_span = m_right->Mmap(buffer->right_fb)[0];

            // Stream info should be identical on both streams
            // They are configured with identical parameters
            buffer->info = LibcameraApp::GetStreamInfo(left_stream);

            // Send the frame to the requester on the same port
            frame_out(0, buffer->id);
        }

        log_ACTIVITY_LO_CameraStopping();
        m_left->StopCamera();
        m_right->StopCamera();
    }

    CamBuffer *Cam::get_buffer()
    {
        m_buffer_mutex.lock();
        for (auto &buf: m_buffers)
        {
            if (!buf.in_use())
            {
                buf.incref();
                m_buffer_mutex.unlock();
                return &buf;
            }
        }

        m_buffer_mutex.unlock();
        return nullptr;
    }

    void Cam::start()
    {
        if (m_streaming)
        {
            log_WARNING_LO_CameraBusy();
        }

        m_left->StopCamera();
        m_right->StopCamera();

        log_ACTIVITY_LO_CameraStarting();
        m_streaming = true;
        m_left->StartCamera();
        m_right->StartCamera();
    }

    void Cam::stop()
    {
        log_ACTIVITY_LO_CameraStopping();
        m_left->StopCamera();
        m_right->StopCamera();
        m_streaming = false;
    }

    void Cam::get_config(CameraConfig &config)
    {
        Fw::ParamValid valid;

#define GET_PARAM(type, pname, vname) \
        do { \
            auto temp = static_cast<type>(paramGet_##pname(valid)); \
            if (valid == Fw::ParamValid::VALID) \
            { \
                config.vname = temp; \
            } \
        } while(0)
#define GET_PARAM_ENUM(type, pname, vname) \
        do { \
            auto temp = paramGet_##pname(valid); \
            if (valid == Fw::ParamValid::VALID) \
            { \
                config.vname = static_cast<type>(temp.e); \
            } \
        } while(0)

        GET_PARAM(U32, FRAME_RATE, frame_rate);
        GET_PARAM(U32, EXPOSURE_TIME, exposure_time);
        GET_PARAM(F32, GAIN, gain);
        GET_PARAM_ENUM(CameraConfig::AeMeteringModeEnum, METERING_MODE, metering_mode);
        GET_PARAM_ENUM(CameraConfig::AeExposureModeEnum, EXPOSURE_MODE, exposure_mode);
        GET_PARAM_ENUM(CameraConfig::AwbModeEnum, AWB, awb);
        GET_PARAM(F32, EV, ev);
        GET_PARAM(F32, AWB_GAIN_R, awb_gain_r);
        GET_PARAM(F32, AWB_GAIN_B, awb_gain_b);
        GET_PARAM(F32, BRIGHTNESS, brightness);
        GET_PARAM(F32, CONTRAST, contrast);
        GET_PARAM(F32, SATURATION, saturation);
        GET_PARAM(F32, SHARPNESS, sharpness);
    }

    void Cam::startStreamThread(const Fw::StringBase &name)
    {
        m_task.start(name, streaming_thread_entry, this);
    }

    void Cam::streaming_thread_entry(void *this_)
    {
        reinterpret_cast<Cam *>(this_)->streaming_thread();
    }

    void Cam::quitStreamThread()
    {
        // Stop the stream
        m_left->StopCamera();
        m_right->StopCamera();

        // Wait for the camera to exit
        m_left->Quit();
        m_right->Quit();
        m_task.join(nullptr);

        m_left->CloseCamera();
        m_right->CloseCamera();

        delete m_left;
        delete m_right;

        m_left = nullptr;
        m_right = nullptr;
    }

    void Cam::STOP_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        stop();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Cam::START_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        start();
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Cam::parametersLoaded()
    {
        CamComponentBase::parametersLoaded();
        parameterUpdated();
    }

    void Cam::parameterUpdated()
    {
        // TODO(tumbar) Support asymmetric camera config
        CameraConfig config;
        get_config(config);
        log_ACTIVITY_LO_CameraConfiguring();
        m_left->ConfigureCamera(config);
        m_right->ConfigureCamera(config);
    }

    bool Cam::frameGet_handler(NATIVE_INT_TYPE portNum, U32 frameId, CamFrame &left, CamFrame &right)
    {
        FW_ASSERT(frameId < CAMERA_BUFFER_N, frameId);
        const auto &buf = m_buffers[frameId];

        if (!buf.in_use())
        {
            log_WARNING_LO_CameraInvalidGet(frameId);
            return false;
        }

        left = CamFrame(
                buf.id, buf.left_span.data(),
                buf.left_span.size(),
                buf.info.width, buf.info.height,
                buf.info.stride,
                buf.left_fb->metadata().timestamp / 1000,
                buf.left_fb->planes()[0].fd.get()
        );

        right = CamFrame(
                buf.id, buf.right_span.data(),
                buf.right_span.size(),
                buf.info.width, buf.info.height,
                buf.info.stride,
                buf.right_fb->metadata().timestamp / 1000,
                buf.right_fb->planes()[0].fd.get()
        );

        return true;
    }

    void Cam::incdec_handler(NATIVE_INT_TYPE portNum, U32 frameId, const ReferenceCounter& dir)
    {
        FW_ASSERT(frameId < CAMERA_BUFFER_N, frameId);

        auto &buf = m_buffers[frameId];
        switch(dir.e)
        {
            case ReferenceCounter::INCREMENT:
                if (!buf.in_use())
                {
                    log_WARNING_LO_CameraInvalidIncref(frameId);
                    return;
                }
                buf.incref();
                break;
            case ReferenceCounter::DECREMENT:
                if (!buf.in_use())
                {
                    log_WARNING_LO_CameraInvalidDecref(frameId);
                    return;
                }
                buf.decref();
                break;
        }
    }
}
