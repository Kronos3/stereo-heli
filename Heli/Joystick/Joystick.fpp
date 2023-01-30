module Heli {

    passive component Joystick {

        # -----------------------------
        # General ports
        # -----------------------------

        output port fcMsg: FcMessage

        output port startTimer: StartTimer
        output port stopTimer: StopTimer

        sync input port sendControl: Svc.Cycle

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

        enum AETRChannel {
            UNMAPPED = -1
            ROLL = 0,
            PITCH = 1,
            THROTTLE = 2,
            YAW = 3,
            CH5 = 4,
            CH6 = 5,
            CH7 = 6
            CH8 = 7
        }

        enum ButtonMapType {
            HOLD,
            TOGGLE
        }

        enum AxisMapType {
            DIRECT,         @< Direct mapping of axis value to channel
            DERIVATIVE,     @< ON value is applied every controller update, OFF value is applied if axis is 0
        }

        # -----------------------------
        # Commands
        # -----------------------------

        @ Start joystick control of the aircraft
        sync command START(
            js: U8              @< Joystick number, /dev/input/js[n]
            trigger_ms: U8,     @< Millisecond period when sending joystick controls
            delay_s: U32,       @< Delay on sending the first control packet
            failsafe_ms: U32    @< Watchdog timeout on Fc for failsafe trigger, see Failsafe tab in configurator
        )

        @ Stop the joystick control of the aircraft
        sync command STOP()

        @ Remap a controller axis to an Fc channel
        sync command MAP_AXIS(
            axis: Axis,     @< Axis to remap
            channel: AETRChannel, @< Channel to map to, -1 for unmapped
            dead_zone: U16  @< Absolute value of dead_zone start
            min_value: U16  @< Fc mapped channel value @ low value
            max_value: U16, @< Fc mapped channel value @ high value
            invert: bool    @< Invert the mapping on this axis
        )

        @ Map an axis to the derivative of an Fc channel (tick rate used at joystick start)
        sync command MAP_AXIS_DERIVATIVE(
            axis: Axis,     @< Axis to remap
            channel: AETRChannel, @< Channel to map to, -1 for unmapped
            dead_zone: U16  @< Absolute value of dead_zone start
            min_value: U16  @< Fc mapped channel value @ low value
            max_value: U16  @< Fc mapped channel value @ high value
            delta_scale: F32  @< Scalar factor to convert stick position to delta
            delta_offset: F32 @< Scalar offset to convert stick position to delta
        )

        @ Remap a controller button to an Fc channel
        sync command MAP_BUTTON(
            button: Button,     @< Button to remap
            channel: AETRChannel, @< Channel to map to, -1 for unmapped
            $type: ButtonMapType, @< Map button using hold or toggle
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

        event ControllerOpenFailed(filename: string size 80, err: I32) \
            severity warning high \
            format "Failed to open joystick {}: {}"

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

        event AxisMapped(axis: Axis, channel: AETRChannel) \
            severity activity low \
            format "Axis {} mapped to channel {}"

        event AxisUnmapped(axis: Axis) \
            severity activity low \
            format "Unmapped axis {}"

        event ButtonMapped(button: Button, channel: AETRChannel) \
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
