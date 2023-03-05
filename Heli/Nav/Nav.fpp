module Heli {

    active component Nav {

        # -----------------------------
        # General ports
        # -----------------------------

        output port frameGet: FrameGet

        async input port frame: Frame
        output port frameOut: Frame

        output port getPosition: PositionGet
        output port getAttitude: AttitudeGet
        output port getQuality: QualityGet

        output port fcMsg: FcMessage

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

        @ Begin tracking motion given rectified stereo images
        async command TRACK()
        async command STOP()

    }

}
