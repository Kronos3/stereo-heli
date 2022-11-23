/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * h264_encoder.cpp - h264 video encoder.
 */

#ifndef TGT_OS_TYPE_DARWIN

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

#include <chrono>
#include <iostream>
#include <unistd.h>
#include <cstring>

#include "h264_encoder.hpp"

static int xioctl(int fd, unsigned long ctl, void* arg)
{
    int ret, num_tries = 10;
    do
    {
        ret = ioctl(fd, ctl, arg);
    } while (ret == -1 && errno == EINTR && num_tries-- > 0);
    return ret;
}

static int get_v4l2_colorspace(std::optional<libcamera::ColorSpace> const &cs)
{
    if (cs == libcamera::ColorSpace::Rec709)
        return V4L2_COLORSPACE_REC709;
    else if (cs == libcamera::ColorSpace::Smpte170m)
        return V4L2_COLORSPACE_SMPTE170M;

    std::cerr << "H264: surprising colour space: " << libcamera::ColorSpace::toString(cs) << std::endl;
    return V4L2_COLORSPACE_SMPTE170M;
}

H264Encoder::H264Encoder(StreamInfo const &info)
        : Encoder(), abortPoll_(false), abortOutput_(false)
{
    // First open the encoder device. Maybe we should double-check its "caps".

    const char device_name[] = "/dev/video11";
    fd_ = open(device_name, O_RDWR, 0);
    if (fd_ < 0)
        throw std::runtime_error("failed to open V4L2 H264 encoder");

    // Apply any options->

    v4l2_control ctrl = {};

    ctrl.id = V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER;
    ctrl.value = 1;
    if (xioctl(fd_, VIDIOC_S_CTRL, &ctrl) < 0)
        throw std::runtime_error("failed to set inline headers");

    // Set the output and capture formats. We know exactly what they will be.

    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.width = info.width;
    fmt.fmt.pix_mp.height = info.height;
    // We assume YUV420 here, but it would be nice if we could do something
    // like info.pixel_format.toV4L2Fourcc();
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = info.stride;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.colorspace = get_v4l2_colorspace(info.colour_space);
    fmt.fmt.pix_mp.num_planes = 1;
    if (xioctl(fd_, VIDIOC_S_FMT, &fmt) < 0)
        throw std::runtime_error("failed to set output format");

    fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.width = info.width;
    fmt.fmt.pix_mp.height = info.height;
    fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
    fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
    fmt.fmt.pix_mp.colorspace = V4L2_COLORSPACE_DEFAULT;
    fmt.fmt.pix_mp.num_planes = 1;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = 0;
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = 512 << 10;
    if (xioctl(fd_, VIDIOC_S_FMT, &fmt) < 0)
        throw std::runtime_error("failed to set capture format");

    // Request that the necessary buffers are allocated. The output queue
    // (input to the encoder) shares buffers from our caller, these must be
    // DMABUFs. Buffers for the encoded bitstream must be allocated and
    // m-mapped.

    v4l2_requestbuffers reqbufs = {};
    reqbufs.count = NUM_OUTPUT_BUFFERS;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_DMABUF;
    if (xioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        throw std::runtime_error("request for output buffers failed");

    // We have to maintain a list of the buffers we can use when our caller gives
    // us another frame to encode.
    for (unsigned int i = 0; i < reqbufs.count; i++)
        input_buffers_available_.push(i);

    reqbufs = {};
    reqbufs.count = NUM_CAPTURE_BUFFERS;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        throw std::runtime_error("request for capture buffers failed");
    num_capture_buffers_ = reqbufs.count;

    for (unsigned int i = 0; i < reqbufs.count; i++)
    {
        v4l2_plane planes[VIDEO_MAX_PLANES];
        v4l2_buffer buffer = {};
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = i;
        buffer.length = 1;
        buffer.m.planes = planes;
        if (xioctl(fd_, VIDIOC_QUERYBUF, &buffer) < 0)
            throw std::runtime_error("failed to capture query buffer " + std::to_string(i));
        buffers_[i].mem = mmap(0, buffer.m.planes[0].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                               buffer.m.planes[0].m.mem_offset);
        if (buffers_[i].mem == MAP_FAILED)
            throw std::runtime_error("failed to mmap capture buffer " + std::to_string(i));
        buffers_[i].size = buffer.m.planes[0].length;
        // Whilst we're going through all the capture buffers, we may as well queue
        // them ready for the encoder to write into.
        if (xioctl(fd_, VIDIOC_QBUF, &buffer) < 0)
            throw std::runtime_error("failed to queue capture buffer " + std::to_string(i));
    }

    // Enable streaming and we're done.

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (xioctl(fd_, VIDIOC_STREAMON, &type) < 0)
        throw std::runtime_error("failed to start output streaming");
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (xioctl(fd_, VIDIOC_STREAMON, &type) < 0)
        throw std::runtime_error("failed to start capture streaming");

    output_thread_ = std::thread(&H264Encoder::outputThread, this);
    poll_thread_ = std::thread(&H264Encoder::pollThread, this);
}

H264Encoder::~H264Encoder()
{
    abortPoll_ = true;
    poll_thread_.join();
    abortOutput_ = true;
    output_thread_.join();

    // Turn off streaming on both the output and capture queues, and "free" the
    // buffers that we requested. The capture ones need to be "munmapped" first.

    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (xioctl(fd_, VIDIOC_STREAMOFF, &type) < 0)
        std::cerr << "Failed to stop output streaming" << std::endl;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (xioctl(fd_, VIDIOC_STREAMOFF, &type) < 0)
        std::cerr << "Failed to stop capture streaming" << std::endl;

    v4l2_requestbuffers reqbufs = {};
    reqbufs.count = 0;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    reqbufs.memory = V4L2_MEMORY_DMABUF;
    if (xioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        std::cerr << "Request to free output buffers failed" << std::endl;

    for (int i = 0; i < num_capture_buffers_; i++)
        if (munmap(buffers_[i].mem, buffers_[i].size) < 0)
            std::cerr << "Failed to unmap buffer" << std::endl;
    reqbufs = {};
    reqbufs.count = 0;
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd_, VIDIOC_REQBUFS, &reqbufs) < 0)
        std::cerr << "Request to free capture buffers failed" << std::endl;

    close(fd_);
}

void H264Encoder::EncodeBuffer(int fd, size_t size, void* mem, StreamInfo const &info, int64_t timestamp_us)
{
    int index;
    {
        // We need to find an available output buffer (input to the codec) to
        // "wrap" the DMABUF.
        std::lock_guard<std::mutex> lock(input_buffers_available_mutex_);
        if (input_buffers_available_.empty())
            throw std::runtime_error("no buffers available to queue codec input");
        index = input_buffers_available_.front();
        input_buffers_available_.pop();
    }
    v4l2_buffer buf = {};
    v4l2_plane planes[VIDEO_MAX_PLANES] = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.index = index;
    buf.field = V4L2_FIELD_NONE;
    buf.memory = V4L2_MEMORY_DMABUF;
    buf.length = 1;
    buf.timestamp.tv_sec = timestamp_us / 1000000;
    buf.timestamp.tv_usec = timestamp_us % 1000000;
    buf.m.planes = planes;
    buf.m.planes[0].m.fd = fd;
    buf.m.planes[0].bytesused = size;
    buf.m.planes[0].length = size;
    if (xioctl(fd_, VIDIOC_QBUF, &buf) < 0)
        throw std::runtime_error("failed to queue input to codec");
}

void H264Encoder::pollThread()
{
    while (true)
    {
        pollfd p = {fd_, POLLIN, 0};
        int ret = poll(&p, 1, 200);
        {
            std::lock_guard<std::mutex> lock(input_buffers_available_mutex_);
            if (abortPoll_ && input_buffers_available_.size() == NUM_OUTPUT_BUFFERS)
                break;
        }
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("unexpected errno " + std::to_string(errno) + " from poll");
        }
        if (p.revents & POLLIN)
        {
            v4l2_buffer buf = {};
            v4l2_plane planes[VIDEO_MAX_PLANES] = {};
            buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
            buf.memory = V4L2_MEMORY_DMABUF;
            buf.length = 1;
            buf.m.planes = planes;
            int ret = xioctl(fd_, VIDIOC_DQBUF, &buf);
            if (ret == 0)
            {
                // Return this to the caller, first noting that this buffer, identified
                // by its index, is available for queueing up another frame.
                {
                    std::lock_guard<std::mutex> lock(input_buffers_available_mutex_);
                    input_buffers_available_.push(buf.index);
                }
                input_done_callback_(nullptr);
            }

            buf = {};
            memset(planes, 0, sizeof(planes));
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.length = 1;
            buf.m.planes = planes;
            ret = xioctl(fd_, VIDIOC_DQBUF, &buf);
            if (ret == 0)
            {
                // We push this encoded buffer to another thread so that our
                // application can take its time with the data without blocking the
                // encode process.
                int64_t timestamp_us = (buf.timestamp.tv_sec * (int64_t) 1000000) + buf.timestamp.tv_usec;
                OutputItem item = {buffers_[buf.index].mem,
                                   buf.m.planes[0].bytesused,
                                   buf.m.planes[0].length,
                                   buf.index,
                                   !!(buf.flags & V4L2_BUF_FLAG_KEYFRAME),
                                   timestamp_us};
                std::lock_guard<std::mutex> lock(output_mutex_);
                output_queue_.push(item);
                output_cond_var_.notify_one();
            }
        }
    }
}

void H264Encoder::outputThread()
{
    OutputItem item;
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(output_mutex_);
            while (true)
            {
                using namespace std::chrono_literals;
                // Must check the abort first, to allow items in the output
                // queue to have a callback.
                if (abortOutput_ && output_queue_.empty())
                    return;

                if (!output_queue_.empty())
                {
                    item = output_queue_.front();
                    output_queue_.pop();
                    break;
                }
                else
                    output_cond_var_.wait_for(lock, 200ms);
            }
        }

        output_ready_callback_(item.mem, item.bytes_used, item.timestamp_us, item.keyframe);
        v4l2_buffer buf = {};
        v4l2_plane planes[VIDEO_MAX_PLANES] = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = item.index;
        buf.length = 1;
        buf.m.planes = planes;
        buf.m.planes[0].bytesused = 0;
        buf.m.planes[0].length = item.length;
        if (xioctl(fd_, VIDIOC_QBUF, &buf) < 0)
            throw std::runtime_error("failed to re-queue encoded buffer");
    }
}
#endif
