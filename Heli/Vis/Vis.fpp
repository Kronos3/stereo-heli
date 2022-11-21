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
        @ Rectify left and right frames using calibration map
        async command RECTIFY(
            calibration_file: string @< Path to calibration file with XY maps for NAV pair
        )

        enum StereoAlgorithm {
            BLOCK_MATCHING,                 @< Standard stereo block matching for performance
            SEMI_GLOBAL_BLOCK_MATCHING,     @< Semi Global matching by Heiko Hirschmuller
            # VULKAN_BLOCK_MATCHING,          @< Vulkan based block matcher (Not implemented yet)
        }

        param STEREO_BM_PRE_FILTER_CAP: I32 default 29
        param STEREO_BLOCK_SIZE: I32 default 5
        param STEREO_MIN_DISPARITY: I32 default -25
        param STEREO_NUM_DISPARITIES: I32 default 16
        param STEREO_BM_TEXTURE_THRESHOLD: I32 default 100
        param STEREO_BM_UNIQUENESS_RATIO: I32 default 10
        param STEREO_SPECKLE_WINDOW_SIZE: I32 default 100
        param STEREO_SPECKLE_RANGE: I32 default 15

        @ Compute disparity between left and right frames, store disparity in LEFT
        async command STEREO(
            algorithm: StereoAlgorithm, @< Stereo matching algorithm
        )

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

        event CalibrationFileOpened(calibrationFile: string) \
            severity activity low \
            format "Opened calibration file {}"

        event CalibrationFileFailed(calibrationFile: string) \
            severity warning low \
            format "Failed to open calibration file {}"
    }

}
