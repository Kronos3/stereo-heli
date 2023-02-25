module Heli {

    active component Vis {

        # -----------------------------
        # General ports
        # -----------------------------

        output port frameGet: FrameGet

        async input port frame: Frame
        output port frameOut: Frame


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

        @ Clear the vision pipeline
        async command CLEAR()

        # pipeline stages:

        enum Interpolation {
            NEAREST, @< nearest neighbor interpolation
            LINEAR,  @< Bilinear interpolation
            CUBIC,   @< Bicubic interpolation
        }

        @ Downsample or upsample image
        async command SCALE(
            fx: F32, @< Horizontal axis scaling factor
            fy: F32, @< Vertical axis scaling factor
            interp: Interpolation @< Interpolation method
        )

        @ Set the camera model image shape
        async command MODEL_SIZE(
            width: U32,     @< Image width in pixels
            height: U32     @< Image height in pixels
        )

        @ Set the left camera intrinsic calibration parameters
        async command MODEL_L_K(
            fx: F32,    @< X focal length in pixels
            fy: F32,    @< Y focal length center in pixels
            cx: F32,    @< Optical center X coordinate in pixels
            cy: F32     @< Optical center Y coordinate in pixels
        )

        @ Set the right camera intrinsic calibration parameters
        async command MODEL_R_K(
            fx: F32,    @< X focal length in pixels
            fy: F32,    @< Y focal length center in pixels
            cx: F32,    @< Optical center X coordinate in pixels
            cy: F32     @< Optical center Y coordinate in pixels
        )

        @ Set the left camera distortion parameters
        async command MODEL_L_D(a: F32, b: F32, c: F32, d: F32, e: F32)

        @ Set the right camera distortion parameters
        async command MODEL_R_D(a: F32, b: F32, c: F32, d: F32, e: F32)

        @ Rotation transform from left camera frame to right camera frame
        async command MODEL_R(
            rx: F32,    @< Rotation about the x-axis
            ry: F32,    @< Rotation about the y-axis
            rz: F32     @< Rotation about the z-axis
        )

        @ Translation from the left camera frame to right camera frame
        async command MODEL_T(
            tx: F32,    @< Translation on x-axis in cm
            ty: F32,    @< Translation on y-axis in cm
            tz: F32     @< Translation on z-axis in cm
        )

        @ Rectify left and right frames using calibration map
        async command RECTIFY()

        enum StereoAlgorithm {
            BLOCK_MATCHING,                 @< Standard stereo block matching for performance
            SEMI_GLOBAL_BLOCK_MATCHING,     @< Semi Global matching by Heiko Hirschmuller
            # VULKAN_BLOCK_MATCHING,          @< Vulkan based block matcher (Not implemented yet)
        }

        param STEREO_PRE_FILTER_CAP: I32 default 29
        param STEREO_BLOCK_SIZE: I32 default 5
        param STEREO_MIN_DISPARITY: I32 default -25
        param STEREO_NUM_DISPARITIES: I32 default 16
        param STEREO_UNIQUENESS_RATIO: I32 default 10
        param STEREO_SPECKLE_WINDOW_SIZE: I32 default 100
        param STEREO_SPECKLE_RANGE: I32 default 15

        param STEREO_BM_TEXTURE_THRESHOLD: I32 default 100

        @ Compute disparity between left and right frames, store disparity in LEFT
        async command STEREO(
            algorithm: StereoAlgorithm, @< Stereo matching algorithm
        )

        param DEPTH_LEFT_MASK_PIX: I32 default 96

        @ Project the disparity map into a depth map using camera extrinsics
        async command DEPTH()

        enum ColorMap {
            AUTUMN = 0,
            BONE = 1,
            JET = 2,
            WINTER = 3,
            RAINBOW = 4,
            OCEAN = 5,
            SUMMER = 6,
            SPRING = 7,
            COOL = 8,
            HSV = 9,
            PINK = 10,
            HOT = 11,
            PARULA = 12,
            MAGMA = 13,
            INFERNO = 14,
            PLASMA = 15,
            VIRIDIS = 16,
            CIVIDIS = 17,
            TWILIGHT = 18,
            TWILIGHT_SHIFTED = 19,
            TURBO = 20,
            DEEPGREEN = 21
        }

        @< Apply colormap to frame
        async command COLORMAP(
            colormap: ColorMap, @< Colormap id
            select: CamSelect   @< Which frame to apply colormap to
        )

        # -----------------------------
        # Events
        # -----------------------------
    }

}
