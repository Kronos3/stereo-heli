/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_app.cpp - base class for libcamera apps.
 */

//#include "preview/preview.hpp"

#include <memory>

#include <Heli/Cam/core/libcamera_app.h>
#include <Heli/VideoStreamer/preview/preview.hpp>
#include <CamCfg.hpp>

#include <fcntl.h>

#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
//#include <preview/preview.hpp>
#include <cstring>
#include <Assert.hpp>

namespace Heli
{
    static libcamera::CameraManager camera_manager;

    // If we definitely appear to be running the old camera stack, complain and give up.
    // Everything else, Pi or not, we let through.
    static void check_camera_stack()
    {
        int fd = open("/dev/video0", O_RDWR, 0);
        if (fd < 0)
            return;

        v4l2_capability caps{};
        int ret = ioctl(fd, VIDIOC_QUERYCAP, &caps);
        close(fd);

        if (ret < 0 || strcmp((char*) caps.driver, "bm2835 mmal") != 0)
            return;

        fprintf(stderr, "ERROR: the system appears to be configured for the legacy camera stack\n");
        exit(-1);
    }

    LibcameraApp::LibcameraApp()
    : controls_(controls::controls), last_timestamp_(0),
      stream(nullptr)
    {
        check_camera_stack();
    }

    LibcameraApp::~LibcameraApp()
    {
        StopCamera();
        Teardown();
        CloseCamera();
    }

    std::string const &LibcameraApp::CameraId() const
    {
        return camera_->id();
    }

    void LibcameraApp::OpenCamera(U32 camera)
    {
        int ret = camera_manager.start();
        if (ret)
            throw std::runtime_error("camera manager failed to start, code " + std::to_string(-ret));

        if (camera_manager.cameras().empty())
            throw std::runtime_error("no cameras available");
        if (camera >= camera_manager.cameras().size())
            throw std::runtime_error("selected camera is not available");

        std::string const &cam_id = camera_manager.cameras()[camera]->id();
        camera_ = camera_manager.get(cam_id);
        if (!camera_)
            throw std::runtime_error("failed to find camera " + cam_id);

        if (camera_->acquire())
            throw std::runtime_error("failed to acquire camera " + cam_id);
        camera_acquired_ = true;
    }

    void LibcameraApp::CloseCamera()
    {
        if (camera_acquired_)
            camera_->release();
        camera_acquired_ = false;

        camera_.reset();
    }

    void LibcameraApp::ConfigureCameraStream(Size videoSize,
                                             int rotation,
                                             bool hflip, bool vflip)
    {
        StreamRoles stream_roles = {libcamera::StreamRole::Raw};

        configuration_ = camera_->generateConfiguration(stream_roles);
        if (!configuration_)
            throw std::runtime_error("failed to generate viewfinder configuration");

        // Now we get to override any of the default settings from the options.
        configuration_->at(0).pixelFormat = libcamera::formats::YUV420;
        configuration_->at(0).size = videoSize;

        configuration_->transform = libcamera::Transform::Identity;
        if (rotation == 180)
            configuration_->transform = libcamera::Transform::Rot180 * configuration_->transform;
        else if (rotation == 90)
            configuration_->transform = libcamera::Transform::Rot90 * configuration_->transform;
        else if (rotation == 270)
            configuration_->transform = libcamera::Transform::Rot270 * configuration_->transform;
        if (hflip)
            configuration_->transform = libcamera::Transform::HFlip * configuration_->transform;
        if (vflip)
            configuration_->transform = libcamera::Transform::VFlip * configuration_->transform;

        configuration_->at(0).bufferCount = CAMERA_BUFFER_N;

        setupCapture();

        stream = configuration_->at(0).stream();
    }

    void LibcameraApp::Teardown()
    {
        for (auto &iter: mapped_buffers_)
        {
            // assert(iter.first->planes().size() == iter.second.size());
            // for (unsigned i = 0; i < iter.first->planes().size(); i++)
            for (auto &span: iter.second)
                munmap(span.data(), span.size());
        }
        mapped_buffers_.clear();

        delete allocator_;
        allocator_ = nullptr;

        configuration_.reset();

        frame_buffers_.clear();

        stream = nullptr;
    }

    void LibcameraApp::ConfigureCamera(const CameraConfig& options)
    {
        controls_.clear();

        // Framerate is a bit weird. If it was set programmatically, we go with that, but
        // otherwise it applies only to preview/video modes. For stills capture we set it
        // as long as possible so that we get whatever the exposure profile wants.
        if (!controls_.contains(controls::FrameDurationLimits.id()))
        {
            if (options.frame_rate > 0)
            {
                int64_t frame_time = 1000000 / options.frame_rate; // in us
                controls_.set(controls::FrameDurationLimits, {frame_time, frame_time});
            }
        }

        if (options.exposure_time)
            controls_.set(controls::ExposureTime, options.exposure_time);
        if (options.gain != 0.0)
            controls_.set(controls::AnalogueGain, options.gain);
        controls_.set(controls::AeMeteringMode, options.metering_mode);
        controls_.set(controls::AeExposureMode, options.exposure_mode);
        controls_.set(controls::ExposureValue, options.ev);
        controls_.set(controls::AwbMode, options.awb);
        controls_.set(controls::ColourGains, {options.awb_gain_r, options.awb_gain_b});
        controls_.set(controls::Brightness, options.brightness);
        controls_.set(controls::Contrast, options.contrast);
        controls_.set(controls::Saturation, options.saturation);
        controls_.set(controls::Sharpness, options.sharpness);
    }

    void LibcameraApp::StartCamera()
    {
        if (camera_started_)
        {
            return;
        }

        // This makes all the Request objects that we shall need.
        makeRequests();

        if (camera_->start(&controls_))
            throw std::runtime_error("failed to start camera");

        controls_.clear();
        camera_started_ = true;
        last_timestamp_ = 0;

        camera_->requestCompleted.connect(this, &LibcameraApp::requestComplete);

        for (std::unique_ptr<Request> &request: requests_)
        {
            if (camera_->queueRequest(request.get()) < 0)
                throw std::runtime_error("Failed to queue request");
        }
    }

    void LibcameraApp::StopCamera()
    {
        {
            // We don't want QueueRequest to run asynchronously while we stop the camera.
            std::lock_guard<std::mutex> lock(camera_stop_mutex_);
            if (camera_started_)
            {
                if (camera_->stop())
                    throw std::runtime_error("failed to stop camera");

                camera_started_ = false;
            }
        }

        if (camera_)
            camera_->requestCompleted.disconnect(this, &LibcameraApp::requestComplete);

        // An application might be holding a CompletedRequest, so queueRequest will get
        // called to delete it later, but we need to know not to try and re-queue it.
        completed_requests_.clear();

        msg_queue_.Clear();
        requests_.clear();
        controls_.clear(); // no need for mutex here
    }

    LibcameraApp::Msg LibcameraApp::Wait()
    {
        return msg_queue_.Wait();
    }

    void LibcameraApp::Quit()
    {
        msg_queue_.Post(Msg(MsgType::Quit));
    }

    void LibcameraApp::queueRequest(CompletedRequest* completed_request)
    {
        BufferMap buffers(std::move(completed_request->buffers));

        Request* request = completed_request->request;
        delete completed_request;
        assert(request);

        // This function may run asynchronously so needs protection from the
        // camera stopping at the same time.
        std::lock_guard<std::mutex> stop_lock(camera_stop_mutex_);
        if (!camera_started_)
            return;

        // An application could be holding a CompletedRequest while it stops and re-starts
        // the camera, after which we don't want to queue another request now.
        {
            std::lock_guard<std::mutex> lock(completed_requests_mutex_);
            auto it = completed_requests_.find(completed_request);
            if (it == completed_requests_.end())
                return;
            completed_requests_.erase(it);
        }

        for (auto const &p: buffers)
        {
            if (request->addBuffer(p.first, p.second) < 0)
                throw std::runtime_error("failed to add buffer to request in QueueRequest");
        }

        {
            std::lock_guard<std::mutex> lock(control_mutex_);
            request->controls() = std::move(controls_);
        }

        if (camera_->queueRequest(request) < 0)
            throw std::runtime_error("failed to queue request");
    }

    libcamera::Stream* LibcameraApp::GetStream(StreamInfo* info) const
    {
        *info = GetStreamInfo(stream);
        return stream;
    }

    std::vector<libcamera::Span<uint8_t>> LibcameraApp::Mmap(FrameBuffer* buffer) const
    {
        auto item = mapped_buffers_.find(buffer);
        if (item == mapped_buffers_.end())
        {
            FW_ASSERT(0 && "Failed to find memmaped memory for DMA");
        }
        return item->second;
    }

    void LibcameraApp::SetControls(ControlList &controls)
    {
        std::lock_guard<std::mutex> lock(control_mutex_);
        controls_ = std::move(controls);
    }

    StreamInfo LibcameraApp::GetStreamInfo(Stream const* stream)
    {
        StreamConfiguration const &cfg = stream->configuration();
        StreamInfo info;
        info.width = cfg.size.width;
        info.height = cfg.size.height;
        info.stride = cfg.stride;
        info.pixel_format = stream->configuration().pixelFormat;
        info.colour_space = stream->configuration().colorSpace;
        return info;
    }

    void LibcameraApp::setupCapture()
    {
        // First finish setting up the configuration.

        CameraConfiguration::Status validation = configuration_->validate();
        if (validation == CameraConfiguration::Invalid)
            throw std::runtime_error("failed to valid stream configurations");
        else if (validation == CameraConfiguration::Adjusted)
            std::cerr << "Stream configuration adjusted" << std::endl;

        if (camera_->configure(configuration_.get()) < 0)
            throw std::runtime_error("failed to configure streams");

        // Next allocate all the buffers we need, mmap them and store them on a free list.

        allocator_ = new FrameBufferAllocator(camera_);
        for (StreamConfiguration &config: *configuration_)
        {
            Stream* stream = config.stream();

            if (allocator_->allocate(stream) < 0)
                throw std::runtime_error("failed to allocate capture buffers");

            for (const std::unique_ptr<FrameBuffer> &buffer: allocator_->buffers(stream))
            {
                // "Single plane" buffers appear as multi-plane here, but we can spot them because then
                // planes all share the same fd. We accumulate them so as to mmap the buffer only once.
                size_t buffer_size = 0;
                for (unsigned i = 0; i < buffer->planes().size(); i++)
                {
                    const FrameBuffer::Plane &plane = buffer->planes()[i];
                    buffer_size += plane.length;
                    if (i == buffer->planes().size() - 1 || plane.fd.get() != buffer->planes()[i + 1].fd.get())
                    {
                        void* memory = mmap(nullptr, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
                        std::cout << "mmap(fd=" << plane.fd.get() << ") -> " << (POINTER_CAST) memory << "\n";
                        mapped_buffers_[buffer.get()].push_back(
                                libcamera::Span<uint8_t>(static_cast<uint8_t*>(memory), buffer_size));
                        buffer_size = 0;
                    }
                }
                frame_buffers_[stream].push(buffer.get());
            }
        }
    }

    void LibcameraApp::makeRequests()
    {
        auto free_buffers(frame_buffers_);
        while (true)
        {
            for (StreamConfiguration &config: *configuration_)
            {
                Stream* stream = config.stream();
                if (stream == configuration_->at(0).stream())
                {
                    if (free_buffers[stream].empty())
                    {
                        return;
                    }
                    std::unique_ptr<Request> request = camera_->createRequest();
                    if (!request)
                        throw std::runtime_error("failed to make request");
                    requests_.push_back(std::move(request));
                }
                else if (free_buffers[stream].empty())
                    throw std::runtime_error("concurrent streams need matching numbers of buffers");

                FrameBuffer* buffer = free_buffers[stream].front();
                free_buffers[stream].pop();
                if (requests_.back()->addBuffer(stream, buffer) < 0)
                    throw std::runtime_error("failed to add buffer to request");
            }
        }
    }

    void LibcameraApp::requestComplete(Request* request)
    {
        if (request->status() == Request::RequestCancelled)
            return;

        auto* payload = new CompletedRequest(sequence_++, request);
        {
            std::lock_guard<std::mutex> lock(completed_requests_mutex_);
            completed_requests_.insert(payload);
        }

        // We calculate the instantaneous framerate in case anyone wants it.
        uint64_t timestamp = payload->buffers.begin()->second->metadata().timestamp;
        if (last_timestamp_ == 0 || last_timestamp_ == timestamp)
            payload->framerate = 0;
        else
            payload->framerate = 1e9 / (timestamp - last_timestamp_);
        last_timestamp_ = timestamp;

        // Send out the frame to the users
        msg_queue_.Post(Msg(MsgType::RequestComplete, payload));
    }

}
