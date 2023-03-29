module Heli {
    type Transform;
    type CameraModel;

    enum Fm_Frame {
        NONE,       @< No frame select (identity and reference frame)
        WORLD,      @< World frame
        SITE,       @< Site frame
        LANDING,    @< Landing frame
        MECH,       @< Mechanical frame
        IMU,        @< IMU frame
        CAM_L,      @< Left camera frame (optical center and orientation)
        CAM_R,      @< Right camera frame (optical center and orientation)
    }

    port CoordFrameGetParent(f: Fm_Frame) -> Fm_Frame
    port CoordFrameGet(f: Fm_Frame, respective: Fm_Frame) -> Transform;
    port CoordFrameTransform(f: Fm_Frame, delta: Transform);

    passive component Fm {

        # -----------------------------
        # General ports
        # -----------------------------

        sync input port getParent: CoordFrameGetParent
        sync input port transform: CoordFrameTransform
        sync input port getFrame: CoordFrameGet

        enum Relationship {
            DYNAMIC,        @< Transforming the child will just update the relationship with the parent
            RIGID,          @< Transforming the child will keep the same relationship with the parent. The closest DYNAMIC ancestor will be changed.
        }

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
            f: Fm_Frame, @< Frame to set parent to frame transform
            p: Fm_Frame, @< Parent coordinate frame
            relation: Relationship, @< Child-parent relationship
            tx: F32,    @< Translation on x-axis in cm
            ty: F32,    @< Translation on y-axis in cm
            tz: F32,    @< Translation on z-axis in cm
            rx: F32,    @< Rotation about the x-axis
            ry: F32,    @< Rotation about the y-axis
            rz: F32     @< Rotation about the z-axis
        )

        @ Get the transform between pTf, report as EVR
        sync command GET(
            f: Fm_Frame, @< Target coordinate frame
            p: Fm_Frame, @< Parent coordinate frame
        )

        # -----------------------------
        # Events
        # -----------------------------

        event TransformR(
            p: Fm_Frame, @< Parent coordinate frame
            f: Fm_Frame, @< Target coordinate frame
            rx: F32,    @< Translation on x-axis in cm
            ry: F32,    @< Translation on y-axis in cm
            rz: F32,    @< Translation on z-axis in cm
        ) severity activity high \
          format "{} -> {} R: {} (x) {} (y) {} (z)"

        event TransformT(
            p: Fm_Frame, @< Parent coordinate frame
            f: Fm_Frame, @< Target coordinate frame
            tx: F32,    @< Translation on x-axis in cm
            ty: F32,    @< Translation on y-axis in cm
            tz: F32,    @< Translation on z-axis in cm
        ) severity activity high \
          format "{} -> {} T: {} (x) {} (y) {} (z)"

        event NoCommonParent(
            p: Fm_Frame, @< Parent coordinate frame
            f: Fm_Frame, @< Target coordinate frame
        ) severity warning high \
          format "Cannot form transform between {} -> {}, no common parent"
    }
}
