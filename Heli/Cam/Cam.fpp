module Rpi {

    struct CamFrameBase {
        bufId: U32 @< Buffer id
        data: U64 @< Image data
        bufSize: U32 @< Image size in bytes
        width: U32 @< Width in pixels
        height: U32 @< Height in pixels
        stride: U32 @< Row stride
        timestamp: U64 @< Timestamp in microseconds
        plane: I32 @< DMA file descriptor for DRM preview
    }

    type CamFrame

    port Frame(frameId: U32)
    port FrameGet(frameId: U32, ref frame: CamFrame) -> bool

    passive component Cam {

        # -----------------------------
        # General ports
        # -----------------------------

        @ Increment reference count on frame
        sync input port incref: Frame

        @ Decrement reference count on frame
        sync input port decref: Frame

        @ Output frames
        output port frame: Frame

        @ Get frame data
        sync input port frameGet: FrameGet

        # -----------------------------
        # Special ports
        # -----------------------------

        @ Command receive port
        command recv port CmdDisp

        @ Command registration port
        command reg port CmdReg

        @ Command response port
        command resp port CmdStatus

        @ Event port
        event port Log

        @ Text event port
        text event port LogText

        @ Time get port
        time get port Time

        @ Telemetry port
        telemetry port Tlm

        @ A port for getting parameter values
        param get port ParamGet

        @ A port for setting parameter values
        param set port ParamSet

        # -----------------------------
        # Commands
        # -----------------------------

        @ Capture a single image and save it to a file
        sync command CAPTURE(
                destination: string size 80 @< Path to save file to
                )

        @ Stop active image capture from the camera
        sync command STOP()

        @ Start camera stream
        sync command START()

        # -----------------------------
        # Events
        # -----------------------------

        event CaptureFailed() \
            severity warning low \
            format "Camera capture failed, dropping frame"

        event ImageSaving(destination: string size 80) \
            severity activity low \
            format "Saving image to file {}"

        event CameraBusy() \
            severity warning low \
            format "Camera already streaming or capturing"

        event CameraStarting() \
            severity activity low \
            format "Camera is starting"

        event CameraStopping() \
            severity activity low \
            format "Camera is stopping"

        event CameraConfiguring() \
            severity activity low \
            format "Sending configuration to camera"

        event CameraInvalidGet(bufId: U32) \
            severity warning low \
            format "Attempting to get frame buffer with ID not in use {}"

        event CameraInvalidIncref(bufId: U32) \
            severity warning low \
            format "Attempting to incref on buffer with ID not in use {}"

        event CameraInvalidDecref(bufId: U32) \
            severity warning low \
            format "Attempting to decref on buffer with ID not in use {}"

        @ Camera frame rate
        param FRAME_RATE: U32 default 30

        @ Exposure time, 0 for hardware minimum
        param EXPOSURE_TIME: U32 default 0

        @ Sensor gain, dB
        param GAIN: F32 default 10.0

        enum MeteringMode {
            CENTRE_WEIGHTED
            SPOT
            MATRIX
            CUSTOM
        }

        @ Metering mode
        param METERING_MODE: MeteringMode default MeteringMode.CENTRE_WEIGHTED

        enum ExposureMode {
            NORMAL
            SHORT
            LONG
            CUSTOM
        }

        @ Exposure mode
        param EXPOSURE_MODE: ExposureMode default ExposureMode.NORMAL

        enum AutoWhiteBalance {
            AUTO
            INCANDESCENT
            TUNGSTEN
            FLUORESCENT
            INDOOR
            DAYLIGHT
            CLOUDY
            CUSTOM
        }

        @ Auto white balance
        param AWB: AutoWhiteBalance default AutoWhiteBalance.AUTO

        @ Energy value
        param EV: F32 default 0.0

        @ Auto white balance red gain, dB
        param AWB_GAIN_R: F32 default 0.0

        @ Auto white balance blue gain, dB
        param AWB_GAIN_B: F32 default 0.0

        @ Brightness
        param BRIGHTNESS: F32 default 0.0

        @ Contrast
        param CONTRAST: F32 default 1.0

        @ Saturation, 1 for color, 0 for bw
        param SATURATION: F32 default 1.0

        @ Sharpness
        param SHARPNESS: F32 default 1.0

        enum DenoisingAlgorithm {
            OFF
            FAST
            HIGH_QUALITY
            MINIMAL
            ZSL
        }

        @ Denoising Algorithm
        param DENOISE: DenoisingAlgorithm default DenoisingAlgorithm.OFF

        telemetry FramesCapture: U32 update on change \
            format "{} frames captured"

        telemetry FramesDropped: U32 update on change \
            format "{} frames dropped"
    }

}