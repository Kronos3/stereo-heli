module Heli {

    passive component Display {

        # -----------------------------
        # General ports
        # -----------------------------

        output port i2c: Drv.I2c

        # -----------------------------
        # Special ports
        # -----------------------------

        @ Command receive port
        command recv port CmdDisp

        @ Command registration port
        command reg port CmdReg

        @ Command response port
        command resp port CmdStatus

        @ Time get port
        time get port Time

        @ Event port
        event port Log

        @ Text event port
        text event port LogText

        # -----------------------------
        # Commands
        # -----------------------------

        @ Write some text on a certain line
        sync command WRITE(
                lineIndex: U8 @< Line index starting from 0
                lineText: string size 16
                )

        event InvalidLineNumber(lineNumber: U8, maxLine: U8) \
                    severity warning high \
                    format "Attempting to write on line index {} when we only have {} lines"
    }

}