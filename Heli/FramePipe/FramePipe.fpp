module Heli {

    passive component FramePipe {
        constant PIPELINE_N = 4
        constant PIPELINE_QUEUE_N = 8

        enum Component {
            STREAMER,       @< Streams to screen or over UDP/TCP connection
            VIS,            @< Vision computations (has internal pipeline)
            NAV             @< Correlates local point-cloud with global map
        }

        enum ComponentType {
            DROP_ON_FULL,           @< Drop if frames come in while we are still processing
            WAIT_ON_FULL_ASSERT,    @< Wait for pipeline stage to free up and then send, assert on queue fill
            WAIT_ON_FULL_DROP,      @< Wait for pipeline stage to free up and then send, drop oldest on queue fill
            NO_REPLY,               @< Don't expect a reply from this stage
        }

        # -----------------------------
        # General ports
        # -----------------------------

        output port incdec: FrameRef

        sync input port camFrame: Frame

        output port frame: [PIPELINE_N] Frame
        sync input port frameIn: Frame

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

        @ Clear the frame pipeline
        sync command CLEAR()

        @ Add a new item to the frame pipeline
        sync command PUSH(
            cmp: Component @< Next item in the pipeline
            cmpType: ComponentType @< How to expect inputs and replies from this stage
        )

        @ Validate the pipeline and start accepting frames from the camera
        sync command CHECK()

        @ Start camera stream
        sync command SHOW()

        # -----------------------------
        # Events
        # -----------------------------

        event CheckPassed() \
            severity activity low \
            format "Frame pipeline checks passed validation"

        enum CheckFailure {
            EMPTY,              @< Pipeline is empty
            NO_REPLY_INPUT,     @< Feeding output of reply into input
            FULL,               @< Too many items in pipeline
        }

        event CheckFailed(reason: CheckFailure) \
            severity warning low \
            format "Frame pipeline checks failed: {}"

        event NotValidated() \
            severity warning low \
            format "Frame pipeline has not been validated by 'CHECK'" \
            throttle 1

        event NoMoreSlots() \
            severity warning low \
            format "No more component slots"

        event FrameNotFound(frameId: U32, $port: I32) \
            severity warning low \
            format "Got frame {} reply from port {} that is not in the pipeline"

        event FramePipeline(pipeline: string) \
            severity activity low \
            format "{}"

        event StageAdded(cmp: Component) \
            severity activity low \
            format "Added {} to stage pipeline"
    }
}
