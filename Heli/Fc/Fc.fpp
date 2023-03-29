module Heli {

    type MspMessage

    port FcMessage(ref msg: MspMessage,
                   ctx: I32,
                   reply: Fc.ReplyAction)

    port FcReply(reply: MspMessage, ctx: I32, status: Fc.ReplyStatus)

    active component Fc {

        # Only the relevant messages are implemented
        enum MspMessageId {
            MSP_MSG_NONE = 0,
            MSP_API_VERSION = 1,
            MSP_FC_VARIANT = 2,
            MSP_FC_VERSION = 3,
            MSP_BOARD_INFO = 4,
            MSP_BUILD_INFO = 5,
            MSP_NAME = 10,
            MSP_SET_NAME = 11,
            MSP_STATUS = 101,           @< Read Fc status
            MSP_RAW_IMU = 102,          @< Get raw IMU readings
            MSP_RC = 105,               @< Get current RC command
            MSP_SET_RAW_RC = 200,       @< Send RC command to pilot Fc
            MSP_RAW_GPS = 106,          @< Current GPS stat
            MSP_SET_RAW_GPS = 201,      @< Inject GPS fix
            MSP_ATTITUDE = 108,         @< Current craft attitude
            MSP_ALTITUDE = 109,         @< Current craft altitude
            MSP_ANALOG = 110,           @< Current ADC readings
            MSP_RC_TUNING = 111,        @< Current remote control settings
            MSP_SET_RC_TUNING = 204,    @< Set remote control settings
            MSP_PID = 112,              @< Current PID settings
            MSP_SET_PID = 202,          @< Set PID settings
            MSP_ACC_CALIBRATION = 205,  @< Used to calibrate the accelerometer

            MSP2_INAV_VISION_POSE = 0x4500,     @< Get current aircraft pose (pos,vel,attitude)
            MSP2_INAV_VISION_POSE_SET = 0x4501, @< Feed attitude, position, velocity to controller
            MSP2_INAV_VISION_PRM_SET = 0x4502,  @< Set weight parameters for vision inputs
        }

        enum MspPIDItem {
            ROLL,
            PITCH,
            YAW,
            ALT,
            POS,
            POSR,
            NAVR,
            LEVEL,
            MAG,
            VEL
        }

        enum ReplyAction {
            REPLY,      @< Send response to message sender
            IGNORE,     @< Don't send reply to sender (setter function)
            NO_REPLY,   @< Tell iNAV not to send a reply, doesn't bog down serial line
        }

        enum ReplyStatus {
            OK,             @< Message was processed properly
            FULL,           @< No more space in the message queue
            ERROR,          @< Got error reply from Fc
            TIMEOUT,        @< Fc did not reply in time
        }

        @ Send a message to the Fc and await a reply
        sync input port msgIn: [FcMessagePorts] FcMessage

        @ Port for replying to external Fc user when Msp reply comes back
        output port msgReply: [FcMessagePorts] FcReply

        @ Service the waiting Msp messages to detect reply timeout
        @ Drop excess packets since this will flush new data to serial
        async input port schedIn: Svc.Sched drop

        # -----------------------------
        # Serial ports
        # -----------------------------

        @ port to request a buffer from the manager
        output port allocate: Fw.BufferGet
        output port deallocate: Fw.BufferSend

        @ send a UART buffer
        output port send: [FcSerialLines] Drv.ByteStreamSend

        @ receive a buffer from the UART - one of the above
        sync input port $recv: [FcSerialLines] Drv.ByteStreamRecv

        # -----------------------------
        # Message ports
        # -----------------------------

        @ Internal interface to queue MSP coming from the Fc
        internal port mspRecv(serialChannel: I32, msp: MspMessage) \
          priority 1

        internal port mspReply(reply: MspMessage, ctx: I32, status: Fc.ReplyStatus) priority 3

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

        @ Reset the Fc state and attempt to re-establish MSP connection to Fc
        async command RESET()

        @ Enable a single uart line for MSP transmission
        async command ENABLE(
            uart: U8 @< Uart line to enable
        )

        @ Disable a single uart line for MSP transmission
        async command DISABLE(
            uart: U8 @< Uart line to disable
        )

        @ Dump the uart line status to EVRs
        async command GET_LINES()

        # -----------------------------
        # Events
        # -----------------------------

        event DumpedExtraneousBytes(num_bytes: U16, channel: U8) \
            severity warning low \
            format "Dumped {} extraneous bytes from serial channel {}"

        event DisableUart(line: U8) \
            severity activity high \
            format "UART line {} is disabled"

        event EnableUart(line: U8) \
            severity activity high \
            format "UART line {} is enabled"

        event InvalidUartLine(line: U8, numLines: U8) \
            severity warning low \
            format "Uart line {} is not valid, only {} available"

        event ErrorResponse(function: MspMessageId) \
            severity warning low \
            format "Function {} received an error response from the Fc"

        enum PacketError {
            MAGIC,              @< Expecting MSPV2 magic 'X'
            DIRECTION,          @< Expecting valid MSP packet direction
            PAYLOAD_SIZE,       @< Unexpectedly large response payload
            CHECKSUM,           @< Checksum failed to validate
        }

        enum MspPacketType {
            REQUEST = 60,
            RESPONSE = 62,
            ERROR = 33
        }

        event PacketError(reason: PacketError, c: U8) \
            severity warning low \
            format "Fc sent an invalid packet: {} {c}"

        event PacketUnhandled(function: MspMessageId, payload_size: U16) \
            severity warning low \
            format "Fc sent a packet without a handler, function: {}, payload_size: {}"

        event ErrorDuringIdentification(function: MspMessageId) \
            severity warning low \
            format "Replied with error during identification: function: {}"

        event UnsupportedApiVersion(major: U8, minor: U8) \
            severity warning high \
            format "Fc is using an incompatible MSP protocol ({}.{}), expected version 2.5+"

        event UnsupportedIdentifier(ident: string size 8) \
            severity warning high \
            format "Fc is using incompatible firmware '{}', expected 'INAV'"

        enum State {
            NOT_CONNECTED,
            OK,
            BAD_SERIAL,             @< Bad serial connection
            BAD_API_VERSION,        @< Invalid API version
            BAD_FIRMWARE_IDENT,     @< Invalid firmware identifier
        }

        event StateNotGood(reason: State) \
            severity warning high \
            format "Fc state is not ready to send/receive: {}"

        event FcStateChange(state: State) \
            severity activity low \
            format "Fc state has changed to {}"

        event Reset() \
            severity activity high \
            format "Fc state has been reset"

        event ConnectionEstablished(major: U8, minor: U8, ident: string size 8, board_info: string size 8) \
            severity activity high \
            format "Fc connection has been established: API version: {}.{}, Fc firmware: {}, Fc board: {}"

        event SerialRecvError(reason: Drv.RecvStatus) \
            severity warning high \
            format "Fc had a recv() serial error: {}"

        event PayloadTooLarge(got: U16, maximum: U16) \
            severity warning low \
            format "Received a packet from Fc with payload size {}, maximum: {}"

        event MessageQueueFull(numMsg: U32, function: MspMessageId) \
            severity warning low \
            format "Message queue full at {} items while trying to send function {}"

        event UnexpectedMspReply(channel: I32, function: MspMessageId) \
            severity warning low \
            format "Received an unexpected MSP reply on serial channel {}, function {}"

        event MspRequest(channel: I32, function: MspMessageId) \
            severity warning low \
            format "Got a MSP request on serial channel {}, function {}. This is a slave device"

        event MspErrorReply(channel: I32, function: MspMessageId) \
            severity warning low \
            format "Received an MSP error reply on serial channel {}, function {}"

        event MspUnmatchedFunctionReply(channel: I32, functionGot: MspMessageId, functionExpected: MspMessageId) \
            severity warning low \
            format "Received an MSP reply with unmatched function on serial channel {}, got {}, expected {}"

        event MspMessageTimeout(channel: I32, function: MspMessageId) \
            severity warning low \
            format "MSP Packet on serial channel {} timed out, function {}"

        # ----------------------
        # Telemetry
        # ----------------------

        @ Number of packets send to Fc
        telemetry Packets: U32 update on change

        @ Number of packets errors from Fc
        telemetry Errors: U32 update on change

        @ Number of bytes sent to Fc
        telemetry BytesSent: U32 update on change

        @ Number of bytes received from Fc
        telemetry BytesRecv: U32 update on change

        @ Number of bytes in MSP queue
        telemetry MspQueueBytes: U32 update on change
    }

}
