/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Google Inc.
 *
 * rkisp1_ipa_serializer.h - Image Processing Algorithm data serializer for rkisp1
 *
 * This file is auto-generated. Do not edit.
 */

#pragma once

#include <tuple>
#include <vector>

#include <libcamera/ipa/rkisp1_ipa_interface.h>
#include <libcamera/ipa/core_ipa_serializer.h>

#include "libcamera/internal/control_serializer.h"
#include "libcamera/internal/ipa_data_serializer.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(IPADataSerializer)

template<>
class IPADataSerializer<ipa::rkisp1::IPAConfigInfo>
{
public:
	static std::tuple<std::vector<uint8_t>, std::vector<SharedFD>>
	serialize(const ipa::rkisp1::IPAConfigInfo &data,
		  ControlSerializer *cs)
	{
		std::vector<uint8_t> retData;

		std::vector<uint8_t> sensorInfo;
		std::tie(sensorInfo, std::ignore) =
			IPADataSerializer<libcamera::IPACameraSensorInfo>::serialize(data.sensorInfo, cs);
		appendPOD<uint32_t>(retData, sensorInfo.size());
		retData.insert(retData.end(), sensorInfo.begin(), sensorInfo.end());

		if (data.sensorControls.size() > 0) {
			std::vector<uint8_t> sensorControls;
			std::tie(sensorControls, std::ignore) =
				IPADataSerializer<ControlInfoMap>::serialize(data.sensorControls, cs);
			appendPOD<uint32_t>(retData, sensorControls.size());
			retData.insert(retData.end(), sensorControls.begin(), sensorControls.end());
		} else {
			appendPOD<uint32_t>(retData, 0);
		}

		return {retData, {}};
	}

	static ipa::rkisp1::IPAConfigInfo
	deserialize(std::vector<uint8_t> &data,
		    ControlSerializer *cs)
	{
		return IPADataSerializer<ipa::rkisp1::IPAConfigInfo>::deserialize(data.cbegin(), data.cend(), cs);
	}


	static ipa::rkisp1::IPAConfigInfo
	deserialize(std::vector<uint8_t>::const_iterator dataBegin,
		    std::vector<uint8_t>::const_iterator dataEnd,
		    ControlSerializer *cs)
	{
		ipa::rkisp1::IPAConfigInfo ret;
		std::vector<uint8_t>::const_iterator m = dataBegin;

		size_t dataSize = std::distance(dataBegin, dataEnd);

		if (dataSize < 4) {
			LOG(IPADataSerializer, Error)
				<< "Failed to deserialize " << "sensorInfoSize"
				<< ": not enough data, expected "
				<< (4) << ", got " << (dataSize);
			return ret;
		}
		const size_t sensorInfoSize = readPOD<uint32_t>(m, 0, dataEnd);
		m += 4;
		dataSize -= 4;
		if (dataSize < sensorInfoSize) {
			LOG(IPADataSerializer, Error)
				<< "Failed to deserialize " << "sensorInfo"
				<< ": not enough data, expected "
				<< (sensorInfoSize) << ", got " << (dataSize);
			return ret;
		}
		ret.sensorInfo =
			IPADataSerializer<libcamera::IPACameraSensorInfo>::deserialize(m, m + sensorInfoSize, cs);
		m += sensorInfoSize;
		dataSize -= sensorInfoSize;


		if (dataSize < 4) {
			LOG(IPADataSerializer, Error)
				<< "Failed to deserialize " << "sensorControlsSize"
				<< ": not enough data, expected "
				<< (4) << ", got " << (dataSize);
			return ret;
		}
		const size_t sensorControlsSize = readPOD<uint32_t>(m, 0, dataEnd);
		m += 4;
		dataSize -= 4;
		if (dataSize < sensorControlsSize) {
			LOG(IPADataSerializer, Error)
				<< "Failed to deserialize " << "sensorControls"
				<< ": not enough data, expected "
				<< (sensorControlsSize) << ", got " << (dataSize);
			return ret;
		}
		if (sensorControlsSize > 0)
			ret.sensorControls =
				IPADataSerializer<ControlInfoMap>::deserialize(m, m + sensorControlsSize, cs);

		return ret;
	}

	static ipa::rkisp1::IPAConfigInfo
	deserialize(std::vector<uint8_t> &data,
		    [[maybe_unused]] std::vector<SharedFD> &fds,
		    ControlSerializer *cs = nullptr)
	{
		return IPADataSerializer<ipa::rkisp1::IPAConfigInfo>::deserialize(data.cbegin(), data.cend(), cs);
	}

	static ipa::rkisp1::IPAConfigInfo
	deserialize(std::vector<uint8_t>::const_iterator dataBegin,
		    std::vector<uint8_t>::const_iterator dataEnd,
		    [[maybe_unused]] std::vector<SharedFD>::const_iterator fdsBegin,
		    [[maybe_unused]] std::vector<SharedFD>::const_iterator fdsEnd,
		    ControlSerializer *cs = nullptr)
	{
		return IPADataSerializer<ipa::rkisp1::IPAConfigInfo>::deserialize(dataBegin, dataEnd, cs);
	}
};


} /* namespace libcamera */