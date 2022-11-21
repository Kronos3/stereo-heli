
#ifndef HELI_LIBCAMERA_APP_H
#define HELI_LIBCAMERA_APP_H

/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020-2021, Raspberry Pi (Trading) Ltd.
 *
 * libcamera_app.hpp - base class for libcamera apps.
 */

#include <sys/mman.h>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <variant>

#include <libcamera/base/span.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/property_ids.h>

#include <core/completed_request.hpp>
//#include <core/post_processor.hpp>
#include <core/stream_info.hpp>
#include <CameraConfig.hpp>
#include <map>

namespace controls = libcamera::controls;
namespace properties = libcamera::properties;

namespace Heli
{
    class LibcameraApp
    {
    public:
        using Stream = libcamera::Stream;
        using FrameBuffer = libcamera::FrameBuffer;
        using ControlList = libcamera::ControlList;
        using Request = libcamera::Request;
        using CameraManager = libcamera::CameraManager;
        using Camera = libcamera::Camera;
        using CameraConfiguration = libcamera::CameraConfiguration;
        using FrameBufferAllocator = libcamera::FrameBufferAllocator;
        using StreamRole = libcamera::StreamRole;
        using StreamRoles = libcamera::StreamRoles;
        using PixelFormat = libcamera::PixelFormat;
        using StreamConfiguration = libcamera::StreamConfiguration;
        using BufferMap = Request::BufferMap;
        using Size = libcamera::Size;
        using Rectangle = libcamera::Rectangle;

        enum class MsgType
        {
            RequestComplete,
            Quit
        };
        typedef CompletedRequest *MsgPayload;

        struct Msg
        {
            explicit Msg(MsgType const &t) : type(t)
            {}

            template<typename T>
            Msg(MsgType const &t, T p) : type(t), payload(std::forward<T>(p))
            {
            }

            MsgType type;
            MsgPayload payload = nullptr;
        };

        LibcameraApp();

        virtual ~LibcameraApp();

        std::string const &CameraId() const;

        void OpenCamera(U32 camera);

        void CloseCamera();

        void ConfigureCameraStream(Size videoSize,
                                   int rotation = 0,
                                   bool hflip = false,
                                   bool vflip = false);

        void Teardown();

        void ConfigureCamera(const CameraConfig &options);

        void StartCamera();

        void StopCamera();

        Msg Wait();

        void Quit();

        Stream* GetStream(StreamInfo *info = nullptr) const;

        std::vector<libcamera::Span<uint8_t>> Mmap(FrameBuffer *buffer) const;

        void SetControls(ControlList &controls);

        static StreamInfo GetStreamInfo(Stream const *stream);

    protected:
//        std::unique_ptr<Options> options_;

    private:
        template<typename T>
        class MessageQueue
        {
        public:
            template<typename U>
            void Post(U &&msg)
            {
                std::unique_lock<std::mutex> lock(mutex_);
                queue_.push(std::forward<U>(msg));
                cond_.notify_one();
            }

            T Wait()
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait(lock, [this]
                { return !queue_.empty(); });
                T msg = std::move(queue_.front());
                queue_.pop();
                return msg;
            }

            void Clear()
            {
                std::unique_lock<std::mutex> lock(mutex_);
                queue_ = {};
            }

        private:
            std::queue<T> queue_;
            std::mutex mutex_;
            std::condition_variable cond_;
        };

        struct PreviewItem
        {
            PreviewItem() : stream(nullptr)
            {}

            PreviewItem(CompletedRequestPtr &b, Stream *s) : completed_request(b), stream(s)
            {}

            PreviewItem &operator=(PreviewItem &&other) noexcept
            {
                completed_request = std::move(other.completed_request);
                stream = other.stream;
                other.stream = nullptr;
                return *this;
            }

            CompletedRequestPtr completed_request;
            Stream *stream;
        };

        void setupCapture();

        void makeRequests();

    public:
        void queueRequest(CompletedRequest *completed_request);

    private:

        void requestComplete(Request *request);

        std::shared_ptr<Camera> camera_;
        bool camera_acquired_ = false;
        std::unique_ptr<CameraConfiguration> configuration_;
        std::map<FrameBuffer *, std::vector<libcamera::Span<uint8_t>>> mapped_buffers_;
        Stream* stream = nullptr;
        FrameBufferAllocator *allocator_ = nullptr;
        std::map<Stream *, std::queue<FrameBuffer *>> frame_buffers_;
        std::vector<std::unique_ptr<Request>> requests_;
        std::mutex completed_requests_mutex_;
        std::set<CompletedRequest *> completed_requests_;
        bool camera_started_ = false;
        std::mutex camera_stop_mutex_;
        MessageQueue<Msg> msg_queue_;
        // Related to the preview window.
//        std::unique_ptr<Preview> preview_;
        std::map<int, CompletedRequestPtr> preview_completed_requests_;
        std::mutex preview_mutex_;
        std::mutex preview_item_mutex_;
        PreviewItem preview_item_;
        std::condition_variable preview_cond_var_;
        bool preview_abort_ = false;
        uint32_t preview_frames_displayed_ = 0;
        uint32_t preview_frames_dropped_ = 0;
        std::thread preview_thread_;
        // For setting camera controls.
        std::mutex control_mutex_;
        ControlList controls_;
        // Other:
        uint64_t last_timestamp_;
        uint64_t sequence_ = 0;
//        PostProcessor post_processor_;
    };
}


#endif //HELI_LIBCAMERA_APP_H
