module Heli {
    type Transform;

    enum CoordinateFrame {
        NONE,       @< No frame select (identity and reference frame)
        WORLD,      @< World frame
        MECH,       @< Mechanical frame
        IMU,        @< IMU frame
        CAM_L,      @< Left camera frame (optical center and orientation)
        CAM_R,      @< Right camera frame (optical center and orientation)
    }

    port CoordFrameSetParent(f: CoordinateFrame, parent: CoordinateFrame, ref t: Transform);
    port CoordFrameGetParent(f: CoordinateFrame) -> CoordinateFrame
    port CoordFrameSet(f: CoordinateFrame, ref t: Transform);
    port CoordFrameGet(f: CoordinateFrame, respective: CoordinateFrame) -> Transform;

    passive component Fm {

        # -----------------------------
        # General ports
        # -----------------------------

        sync input port setParent: CoordFrameSetParent
        sync input port getParent: CoordFrameGetParent
        sync input port setFrame: CoordFrameSet
        sync input port getFrame: CoordFrameGet

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

        @ Set the transform between pTf
        sync command SET(
            f: CoordinateFrame, @< Frame to set parent to frame transform
            p: CoordinateFrame, @< Parent coordinate frame
            tx: F32,    @< Translation on x-axis in cm
            ty: F32,    @< Translation on y-axis in cm
            tz: F32,    @< Translation on z-axis in cm
            rx: F32,    @< Rotation about the x-axis
            ry: F32,    @< Rotation about the y-axis
            rz: F32     @< Rotation about the z-axis
        )

        @ Get the transform between pTf, report as EVR
        sync command GET(
            f: CoordinateFrame, @< Target coordinate frame
            p: CoordinateFrame, @< Parent coordinate frame
        )

        # -----------------------------
        # Events
        # -----------------------------

        event TransformR(
            p: CoordinateFrame, @< Parent coordinate frame
            f: CoordinateFrame, @< Target coordinate frame
            rx: F32,    @< Translation on x-axis in cm
            ry: F32,    @< Translation on y-axis in cm
            rz: F32,    @< Translation on z-axis in cm
        ) severity activity high \
          format "{} -> {} R: {} (x) {} (y) {} (z)"

        event TransformT(
            p: CoordinateFrame, @< Parent coordinate frame
            f: CoordinateFrame, @< Target coordinate frame
            tx: F32,    @< Translation on x-axis in cm
            ty: F32,    @< Translation on y-axis in cm
            tz: F32,    @< Translation on z-axis in cm
        ) severity activity high \
          format "{} -> {} T: {} (x) {} (y) {} (z)"

        event NoCommonParent(
            p: CoordinateFrame, @< Parent coordinate frame
            f: CoordinateFrame, @< Target coordinate frame
        ) severity warning high \
          format "Cannot form transform between {} -> {}, no common parent"
    }
}
