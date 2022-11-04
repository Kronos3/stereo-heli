/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * output.cpp - video stream output base class
 */

#include <cinttypes>
#include "output.hpp"

Output::Output()
        : state_(WAITING_KEYFRAME), enable_(true), time_offset_(0),
        last_timestamp_(0)
{
}

void Output::Signal()
{
    enable_ = !enable_;
}

void Output::OutputReady(void* mem, size_t size, int64_t timestamp_us, bool keyframe)
{
    // When output is enabled, we may have to wait for the next keyframe.
    uint32_t flags = keyframe ? FLAG_KEYFRAME : FLAG_NONE;
    if (!enable_)
        state_ = DISABLED;
    else if (state_ == DISABLED)
        state_ = WAITING_KEYFRAME;
    if (state_ == WAITING_KEYFRAME && keyframe)
        state_ = RUNNING, flags |= FLAG_RESTART;
    if (state_ != RUNNING)
        return;

    // Frig the timestamps to be continuous after a pause.
    if (flags & FLAG_RESTART)
        time_offset_ = timestamp_us - last_timestamp_;
    last_timestamp_ = timestamp_us - time_offset_;

    outputBuffer(mem, size, last_timestamp_, flags);
}

void Output::outputBuffer(void* mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
    // Supply this so that a vanilla Output gives you an object that outputs no buffers.
}
