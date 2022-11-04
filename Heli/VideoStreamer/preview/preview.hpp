/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * preview.hpp - preview window interface
 */

#pragma once

#include <functional>
#include <string>

#include <libcamera/base/span.h>
#include <Rpi/Cam/CamFrame.hpp>

#include "Rpi/Cam/core/stream_info.hpp"
#include "libcamera/geometry.h"

class Preview
{
public:
	typedef std::function<void(int fd)> DoneCallback;

	Preview() {} //  : options_(options) {}
	virtual ~Preview() {}
	// This is where the application sets the callback it gets whenever the viewfinder
	// is no longer displaying the buffer and it can be safely recycled.
	virtual void SetInfoText(const std::string &text) {}
	// Display the buffer. You get given the fd back in the BufferDoneCallback
	// once its available for re-use.
	virtual void Show(const Rpi::CamFrame& frame) = 0;
	// Reset the preview window, clearing the current buffers and being ready to
	// show new ones.
	virtual void Reset() = 0;
	// Check if preview window has been shut down.
	virtual bool Quit() { return false; }
	// Return the maximum image size allowed.
	virtual void MaxImageSize(unsigned int &w, unsigned int &h) const = 0;
};

libcamera::Size get_screen_size();

Preview *make_drm_preview();
