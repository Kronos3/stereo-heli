module Heli {

    active component VideoStreamer {

        # -----------------------------
        # General ports
        # -----------------------------

        output port incdec: FrameRef
        output port frameGet: FrameGet

        @ Output frames
        async input port frame: Frame

        async input port sched: Svc.Sched

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

        # -----------------------------
        # Commands
        # -----------------------------

        @ Configures where UDP video is sent to
        async command NETWORK_SEND(
            address: string size 32 @< Address to UDP listener
            portN: U16 @< Port number
            )

        enum DisplayLocation {
            NONE @< Dump the frames immediately
            HDMI @< Display video stream over on-board HDMI port
            UDP @< Stream video over UDP to HOSTNAME:PORT
            BOTH @< Stream to both UDP and HDMI simultaneously
        }

        @ Start camera stream
        async command DISPLAY(
            where: DisplayLocation @< Where to display frames
            eye: CamSelect @< Which camera to stream
            )

        enum ImageEncoding {
            JPEG,   @< JPEG Image (Loss Compression)
            PNG,    @< Lossy compression with alpha channel
            TIFF    @< Lossless compression which can store multiple images in a single file
        }

        @ Save the next camera frame that VideoStreamer receives to disk
        async command CAPTURE(
            location: string size 120 @< Where to save the image, should not include the extension
            eye: CamSelect @< Which camera to save image from. If BOTH, two files are saved, unless TIFF
            encoding: ImageEncoding @< Image compression format - selects file extension
        )

        # -----------------------------
        # Events
        # -----------------------------

        event StreamingTo(where: DisplayLocation) \
            severity activity low \
            format "Now streaming to {}"

        event InvalidFrameBuffer(frameId: U32) \
            severity warning low \
            format "Received an invalid frame id {}"

        event EyeNotSupport() \
            severity warning low \
            format "Cannot display both eyes"

        event CaptureTimeout(
            destination: string size 120
        ) severity warning low \
          format "Timeout occurred during capture of {}"

        event CaptureCompleted(
            camera: CamSelect,
            destination: string size 120
        ) severity activity low \
          format "Saved capture on {} camera to {}"

        @ Frame output rate
        telemetry FramesPerSecond: U32 format "{} fps"

        @ Total number of frames sent to the video streamer
        telemetry FramesTotal: U32

        @ Number of UDP packets sent to server
        telemetry PacketsSent: U32
    }

}