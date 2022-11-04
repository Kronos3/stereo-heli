module Rpi {

    port Ready() -> void

    passive component FramePipe {

        # -----------------------------
        # General ports
        # -----------------------------

        sync input port ready: Ready
        output port incref: Frame
        output port decref: Frame

        sync input port frame: Frame
        output port frame: Frame

    }

}