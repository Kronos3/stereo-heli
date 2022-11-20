module Heli {

    port Ready()

    passive component FramePipe {

        # -----------------------------
        # General ports
        # -----------------------------

        sync input port ready: Ready
        output port incdec: FrameRef

        sync input port frame: Frame
        output port frameOut: Frame

    }

}