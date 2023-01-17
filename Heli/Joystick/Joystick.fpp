module Heli {

    passive component Joystick {

        # -----------------------------
        # General ports
        # -----------------------------

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

        enum Axis {
            LX = 0,
            LY = 1,
            L2 = 2,
            RX = 3,
            RY = 4,
            R2 = 5,
            HORIZONTAL = 6,
            VERTICAL = 7,
        }

        enum Button {
            X = 0,
            CIRCLE = 1,
            TRIANGLE = 2,
            SQUARE = 3,
            L1 = 4,
            R1 = 5,
            L2 = 6,
            R2 = 7,
            SELECT = 8,
            START = 9,
            PS = 10,
            L3 = 11,
            R3 = 12,
        }

        # -----------------------------
        # Commands
        # -----------------------------

        @ Start joystick control of the aircraft
        sync command START(
            js: U8 @< Joystick number, /dev/input/js[n]
        )

        @ Stop the joystick control of the aircraft
        sync command STOP()

        @ Remap a controller axis to an Fc channel
        sync command MAP_AXIS(
            axis: Axis,     @< Axis to remap
            channel: I8     @< Channel to map to, -1 for unmapped
            dead_zone: U16  @< Absolute value of dead_zone start
            min_value: U16  @< Fc mapped channel value @ low value
            max_value: U16  @< Fc mapped channel value @ high value
        )

        @ Remap a controller button to an Fc channel
        sync command MAP_BUTTON(
            button: Button,     @< Button to remap
            channel: I8,        @< Channel to map to, -1 for unmapped
            off_value: I16,     @< Value to use while button not pressed, negative to ignore release
            on_value: I16,      @< Value to use while button is pressed, negative to ignore press
        )

        @ Save a controller mapping profile to disk
        sync command SAVE(
            filename: string size 80  @< File to save mapping to
        )

        @ Load a controller map from disk
        sync command LOAD(
            filename: string size 80 @< File to load mapping from
        )

        enum ControllerMapStage {
            OPEN,
            AXIS,
            BUTTON
        }

        event ControllerMapLoadFailed(
            filename: string size 80,
            stage: ControllerMapStage,
            error: I32
        ) \
            severity warning low \
            format "Failed to load controller mapping from {} during {} error {}"

        event ControllerMapSaveFailed(
            filename: string size 80,
            stage: ControllerMapStage,
            error: I32
        ) \
            severity warning low \
            format "Failed to save controller mapping to {} during {} error {}"

        event SavedControllerMapping(
            filename: string size 80
        ) severity activity low \
          format "Saved joystick controller map to {}"

        event LoadedControllerMapping(
            filename: string size 80
        ) severity activity low \
          format "Loaded joystick controller map from {}"

        event InvalidChannel(channel: I8, max_channel: I8) \
            severity warning low \
            format "Channel ({}) must be between 0 and {} or -1 for unmapped"

        event AxisMapped(axis: Axis, channel: I8) \
            severity activity low \
            format "Axis {} mapped to channel {}"

        event AxisUnmapped(axis: Axis) \
            severity activity low \
            format "Unmapped axis {}"

        event ButtonMapped(button: Button, channel: I8) \
            severity activity low \
            format "Button {} mapped to channel {}"

        event ButtonUnmapped(button: Button) \
            severity activity low \
            format "Unmapped button {}"

        event InvalidAxisNumber(number: U8) \
            severity warning high \
            format "Invalid axis number from controller {}"

        event InvalidButtonNumber(number: U8) \
            severity warning high \
            format "Invalid axis number from controller {}"

        event JoystickAlreadyRunning() \
            severity warning low \
            format "Joystick event listener is already running"

        event JoystickNotRunning() \
            severity warning low \
            format "Joystick event listener is not running"

        event ErrorDuringPoll(error: I32) \
            severity warning low \
            format "Error {} while trying to poll joystick"

        event ErrorDuringRead(error: I32) \
            severity warning low \
            format "Error {} while trying to read from joystick"

        event JoystickStarting(js: U8) \
            severity activity low \
            format "Joystick thread started reading from /dev/input/js{}"

        event JoystickShutdown() \
            severity activity low \
            format "Joystick thread is shutting down"

        event CommandAlreadyExecuting(op: U32) \
            severity warning low \
            format "Opcode {} already executing, only one command at a time supported"
    }

}
