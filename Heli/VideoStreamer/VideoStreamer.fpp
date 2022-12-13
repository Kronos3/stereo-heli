module Heli {

    active component VideoStreamer {

        # -----------------------------
        # General ports
        # -----------------------------

        output port incdec: FrameRef
        output port frameGet: FrameGet

        @ Output frames
        async input port frame: Frame

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

        @ Frame output rate
        telemetry FramesPerSecond: U32 format "{} fps"

        @ Total number of frames sent to the video streamer
        telemetry FramesTotal: U32

        @ Number of UDP packets sent to server
        telemetry PacketsSent: U32
    }

}