module Heli {

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

    enum CamSelect {
        LEFT,
        RIGHT,
        BOTH
    }

    enum ReferenceCounter {
        INCREMENT,
        DECREMENT
    }

    type CamFrame

    port Frame(frameId: U32)
    port FrameRef(frameId: U32, dir: ReferenceCounter)
    port FrameGet(frameId: U32, ref left: CamFrame, ref right: CamFrame) -> bool

    passive component Cam {

        # -----------------------------
        # General ports
        # -----------------------------

        @ Increment/Decrement reference count on frame
        sync input port incdec: FrameRef

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

        @ Stop active image capture from the camera
        sync command STOP()

        @ Start camera stream
        sync command START()

        enum Rotation {
            R_0,        @< 0° rotation
            R_90,       @< 90° rotation
            R_180,      @< 180° rotation
            R_270       @< 270° rotation
        }

        @ Set up the camera stream settings
        sync command CONFIGURE(
            width: U32,         @< Image width, check supported dims by this camera
            height: U32,        @< Image height, check supported dims by this camera
            l_rot: Rotation,    @< Left camera rotation angle
            r_rot: Rotation,    @< Right camera rotation angle
            l_vflip: bool,      @< Vertical flip of left camera
            r_vflip: bool,      @< Vertical flip of right camera
            l_hflip: bool,      @< Horizontal flip of left camera
            r_hflip: bool       @< Horizontal flip of right camera
        )

        # -----------------------------
        # Events
        # -----------------------------

        event CaptureFailed() \
            severity warning low \
            format "Camera capture failed, dropping frame"

        event ImageSaving(destination: string size 80) \
            severity activity low \
            format "Saving image to file {}"

        event CameraActivated(camLeft: string size 40,
                              camRight: string size 40) \
            severity activity low \
            format "Stereo camera activated with IDs left={} right={}"

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

        event CameraStreamConfiguring(width: U32, height: U32) \
            severity activity high \
            format "Initializing camera stream @ {}x{}"

        event CameraConfigError(eye: CamSelect, err: string size 80) \
            severity warning high \
            format "Failed to configure {} camera: {}"

        event CameraStreamNotConfigured() \
            severity warning high \
            format "Camera stream is not initialized"

        event InvalidBuffer(bufId: U32) \
            severity warning low \
            format "Attempting to get invalid frame buffer: {}"

        event BufferNotInUse(bufId: U32) \
            severity warning low \
            format "Attempting to get frame buffer not in use: {}"

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