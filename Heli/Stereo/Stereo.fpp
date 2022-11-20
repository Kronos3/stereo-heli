module Heli {

    active component Stereo {

        # -----------------------------
        # General ports
        # -----------------------------

        output port ready: Ready
        output port incdec: FrameRef
        output port frameGet: FrameGet

        async input port frame: Frame
        output port frameOut: Frame
    }

}
