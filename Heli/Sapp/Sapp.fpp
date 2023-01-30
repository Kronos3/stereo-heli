module Heli {

    struct Quaternion {
        x: F32
        y: F32
        z: F32
        w: F32
    }

    struct EulerAngles {
        roll: F32
        pitch: F32
        yaw: F32
    }

    struct Vector3 {
        x: F32
        y: F32
        z: F32
    }

    @ State estimate quality state
    enum SappQuality {
        INVALID,        @< We have not received a position estimate from Fc in timeout range
        DEAD_RECKON,    @< Position estimate is not refined by vision
        FINE,           @< Best quality position and attitude estimate we can provide
    }

    port PositionGet() -> Vector3
    port AttitudeGet() -> Quaternion
    port QualityGet() -> SappQuality

    active component Sapp {

        # -----------------------------
        # General ports
        # -----------------------------

        sync input port getPosition: PositionGet
        sync input port getAttitude: AttitudeGet
        sync input port getQuality: QualityGet

        output port fcMsg: FcMessage
        async input port fcReply: FcReply

        async input port schedIn: Svc.Sched

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

        @ Start the Sapp state polling
        async command START()

        @ Stop the Sapp state polling
        async command STOP()

        @ Reset the position of the aircraft in both our and Fc estimates
        async command SET_POSITION(x: F32, y: F32, z: F32)

        @ Reset the attitude of the aircraft (world frame)
        async command SET_ATTITUDE(roll_deg: F32, pitch_deg: F32, yaw_deg: F32)

        @ Rate to poll Fc for attitude and position states
        param FC_POLL_RATE_HZ: U32 default 50

        telemetry PositionX: F32 format "{} m"
        telemetry PositionY: F32 format "{} m"
        telemetry PositionZ: F32 format "{} m"

        telemetry QuaternionX: F32
        telemetry QuaternionY: F32
        telemetry QuaternionZ: F32
        telemetry QuaternionW: F32

        telemetry Quality: SappQuality

        event FcPollStarted(rate: U32) \
            severity activity low \
            format "Start polling Fc for attitude and position @ {} Hz"

        event FcPollStopped() \
            severity activity low \
            format "Stopped polling Fc for attitude and position"

        event QualityRefined(quality: SappQuality) \
            severity activity high \
            format "Sapp quality refined to {}"

        event QualityDegraded(quality: SappQuality) \
            severity activity high \
            format "Sapp quality degraded to {}"

        event PositionReset(position: Vector3) \
            severity activity high \
            format "Position reset to {}"

        event AttitudeReset(attitude_euler: EulerAngles, attitude: Quaternion) \
            severity activity high \
            format "Attitude reset to {} ({})"
    }

}
