#ifndef HELI_CAMERACONFIG_H
#define HELI_CAMERACONFIG_H

#include <Fw/Types/BasicTypes.hpp>
#include <libcamera/control_ids.h>

struct CameraConfig
{
    using AeMeteringModeEnum = libcamera::controls::AeMeteringModeEnum;
    using AeExposureModeEnum = libcamera::controls::AeExposureModeEnum;
    using AwbModeEnum = libcamera::controls::AwbModeEnum;
    using NoiseReductionModeEnum = libcamera::controls::draft::NoiseReductionModeEnum;

    U32 frame_rate = 30;                                  //!< Frame-rate cap, 0 for none
    U32 exposure_time = 0;                                //!< Exposure time/shutter speed
    F32 gain = 10.0;                                      //!< Signal gain in dB
    AeMeteringModeEnum metering_mode =
            AeMeteringModeEnum::MeteringCentreWeighted;   //!< Metering mode
    AeExposureModeEnum exposure_mode =
            AeExposureModeEnum::ExposureNormal;           //!< Exposure modes
    AwbModeEnum awb = AwbModeEnum::AwbAuto;               //!< Auto white balance modes
    F32 ev = 0.0;                                         //!< Exposure value
    F32 awb_gain_r = 0.0;                                 //!< Custom mode AWB red gain
    F32 awb_gain_b = 0.0;                                 //!< Custom mode AWB blue gain
    F32 brightness = 0.0;                                 //!< Brightness gain
    F32 contrast = 1.0;                                   //!< Contrast
    F32 saturation = 1.0;                                 //!< Color saturation
    F32 sharpness = 1.0;                                  //!< Sharpness

    NoiseReductionModeEnum denoise =                      //!< De-noising algorithm
            NoiseReductionModeEnum::NoiseReductionModeHighQuality;
};

#endif //HELI_CAMERACONFIG_H
