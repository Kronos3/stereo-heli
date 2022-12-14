module Heli {

    passive component Cadre {

        # -----------------------------
        # General ports
        # -----------------------------

        output port send: Fw.Com

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

        @ Add a new item to the frame pipeline
        sync command IDENTIFY(
            agentId: U8         @< Id number of the agent
        )

        # -----------------------------
        # Events
        # -----------------------------

        event AgentIdentify(agent: U8) \
            severity activity low \
            format "Sending Agent ID {} to GDS"
    }
}
