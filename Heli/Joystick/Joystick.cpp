//
// Created by tumbar on 1/13/23.
//

#include <fcntl.h>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include "Joystick.hpp"
#include "File.hpp"

namespace Heli
{
    Joystick::Joystick(const char* compName)
            : JoystickComponentBase(compName),
              m_joystick(0), is_running(false), cmd_waiting(false),
              m_opcode(0), m_cmdSeq(0), m_fd(-1), channels{},
              inav_msg(Fc_MspMessageId::MSP_SET_RAW_RC)
    {
    }

    void Joystick::init(NATIVE_INT_TYPE instance)
    {
        JoystickComponentBase::init(instance);
    }

    void Joystick::STOP_cmdHandler(U32 opCode, U32 cmdSeq)
    {
        setup_reply(opCode, cmdSeq);

        m_mutex.lock();
        if (!is_running)
        {
            log_WARNING_LO_JoystickNotRunning();
            reply(Fw::CmdResponse::EXECUTION_ERROR);
            m_mutex.unlock();
            return;
        }

        is_running = false;
        m_mutex.unlock();
    }

    void Joystick::START_cmdHandler(U32 opCode, U32 cmdSeq, U8 js)
    {
        setup_reply(opCode, cmdSeq);

        m_mutex.lock();
        if (is_running)
        {
            log_WARNING_LO_JoystickAlreadyRunning();
            reply(Fw::CmdResponse::BUSY);
            m_mutex.unlock();
            return;
        }

        m_joystick = js;
        m_task = Os::Task();

        m_mutex.unlock();

        Fw::String task_name = "JoystickListener";
        m_task.start(task_name, Joystick::main_loop, this);
    }

    void Joystick::main_loop()
    {
        log_ACTIVITY_LO_JoystickStarting(m_joystick);

        Fw::String path;
        path.format("/dev/input/js%d", m_joystick);

        // Blocking call
        m_mutex.lock();
        m_fd = ::open(path.toChar(), O_RDONLY);
        m_mutex.unlock();

        // Reply to the start
        reply(Fw::CmdResponse::OK);

        fd_set rfds;
        timeval tv{};

        // Watch the joystick fd to wait for input
        FD_ZERO(&rfds);
        FD_SET(m_fd, &rfds);

        // Waits up to 3 seconds before unblocking
        // This allows us to safely break out of this loop
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        JoystickEvent event{};
        bool error = false;
        is_running = true;
        while (is_running)
        {
            I32 rc = select(1, &rfds, nullptr, nullptr, &tv);
            if (rc == -1)
            {
                log_WARNING_LO_ErrorDuringPoll(errno);
                error = true;
                break;
            }
            else if (rc)
            {
                size_t r = ::read(m_fd, &event, sizeof(event));
                if (r != sizeof(event))
                {
                    log_WARNING_LO_ErrorDuringRead(errno);
                    error = true;
                    break;
                }
                else
                {
                    handle_event(event);
                }
            }
            else
            {
                // No data was read from joystick
                // Check if we should terminate
            }
        }

        is_running = false;
        log_ACTIVITY_LO_JoystickShutdown();

        ::close(m_fd);
        m_fd = -1;
        reply(error ?
              Fw::CmdResponse::EXECUTION_ERROR
                    : Fw::CmdResponse::OK);
    }

    void Joystick::main_loop(void* this_)
    {
        static_cast<Joystick*>(this_)->main_loop();
    }

    void Joystick::setup_reply(FwOpcodeType opcode, U32 cmdSeq)
    {
        m_mutex.lock();
        if (cmd_waiting)
        {
            log_WARNING_LO_CommandAlreadyExecuting(opcode);
            cmdResponse_out(opcode, cmdSeq, Fw::CmdResponse::BUSY);
            m_mutex.unlock();
            return;
        }

        cmd_waiting = true;
        m_opcode = opcode;
        m_cmdSeq = cmdSeq;
        m_mutex.unlock();
    }

    void Joystick::reply(const Fw::CmdResponse &response)
    {
        m_mutex.lock();
        if (!cmd_waiting)
        {
            // Nothing to reply to
            m_mutex.unlock();
            return;
        }

        cmd_waiting = false;
        cmdResponse_out(m_opcode, m_cmdSeq, response);
        m_opcode = 0;
        m_cmdSeq = 0;
        m_mutex.unlock();
    }

    void Joystick::handle_event(const Joystick::JoystickEvent &event)
    {
        if (event.type & BUTTON)
        {
            if (event.number >= FW_NUM_ARRAY_ELEMENTS(button_mappings))
            {
                log_WARNING_HI_InvalidButtonNumber(event.number);
                return;
            }

            const ButtonMapping &mapping = button_mappings[event.number];
            if (!mapping.is_mapped())
            {
                // Ignore unmapped events
                return;
            }

            mapping.set_fc(event.value, channels[mapping.channel]);
        }
        else if (event.type & AXIS)
        {
            if (event.number >= FW_NUM_ARRAY_ELEMENTS(axis_mappings))
            {
                log_WARNING_HI_InvalidAxisNumber(event.number);
                return;
            }

            const AxisMapping &mapping = axis_mappings[event.number];
            if (!mapping.is_mapped())
            {
                // Ignore unmapped events
                return;
            }

            channels[mapping.channel] = mapping.get_fc(event.value);
        }

        // FIXME(tumbar) Do we want to rate limit the transmission?
        send_control();
    }

    void Joystick::send_control()
    {
        inav_msg.set_payload(reinterpret_cast<const U8*>(&channels), sizeof(channels));
        fcMsg_out(0, inav_msg, 0, Fc_ReplyAction::IGNORE);
    }

    void Joystick::MAP_AXIS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                       Heli::Joystick_Axis axis,
                                       I8 channel, U16 dead_zone,
                                       U16 min_value, U16 max_value)
    {
        AxisMapping &mapping = axis_mappings[axis.e];
        if (channel < 0)
        {
            if (axis_mappings[axis.e].is_mapped())
            {
                mapping.unmap();
                log_ACTIVITY_LO_AxisUnmapped(axis);
            }
        }
        else
        {
            if (channel >= INAV_MSP_RC_CHANNEL)
            {
                log_WARNING_LO_InvalidChannel(channel, INAV_MSP_RC_CHANNEL);
                cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
                return;
            }

            mapping.channel = channel;
            mapping.deadzone = dead_zone;
            mapping.min_value = min_value;
            mapping.max_value = max_value;
            log_ACTIVITY_LO_AxisMapped(axis, channel);
        }

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void
    Joystick::MAP_BUTTON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                    Heli::Joystick_Button button,
                                    I8 channel, I16 off_value, I16 on_value)
    {

        ButtonMapping &mapping = button_mappings[button.e];
        if (channel < 0)
        {
            if (button_mappings[button.e].is_mapped())
            {
                mapping.unmap();
                log_ACTIVITY_LO_ButtonUnmapped(button);
            }
        }
        else
        {
            if (channel >= INAV_MSP_RC_CHANNEL)
            {
                log_WARNING_LO_InvalidChannel(channel, INAV_MSP_RC_CHANNEL);
                cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
                return;
            }

            mapping.channel = channel;
            mapping.off_value = off_value;
            mapping.on_value = on_value;
            log_ACTIVITY_LO_ButtonMapped(button, channel);
        }

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Joystick::SAVE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg &filename)
    {
        Os::File file;
        Os::File::Status status;

        // Open a new file or overwrite an old one
        status = file.open(filename.toChar(), Os::File::Mode::OPEN_WRITE);
        if (status != Os::File::Status::OP_OK)
        {
            log_WARNING_LO_ControllerMapSaveFailed(
                    filename, Joystick_ControllerMapStage::OPEN,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        // Write the axis mappings
        NATIVE_INT_TYPE size = sizeof(axis_mappings);
        status = file.write(&axis_mappings, size);
        if (status != Os::File::Status::OP_OK || size != sizeof(axis_mappings))
        {
            log_WARNING_LO_ControllerMapSaveFailed(
                    filename, Joystick_ControllerMapStage::AXIS,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        // Write the button mappings
        size = sizeof(button_mappings);
        status = file.write(&button_mappings, size);
        if (status != Os::File::Status::OP_OK || size != sizeof(button_mappings))
        {
            log_WARNING_LO_ControllerMapSaveFailed(
                    filename, Joystick_ControllerMapStage::BUTTON,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        file.close();

        log_ACTIVITY_LO_SavedControllerMapping(filename);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Joystick::LOAD_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg &filename)
    {
        Os::File file;
        Os::File::Status status;

        // Open a new file or overwrite an old one
        status = file.open(filename.toChar(), Os::File::Mode::OPEN_READ);
        if (status != Os::File::Status::OP_OK)
        {
            log_WARNING_LO_ControllerMapLoadFailed(
                    filename, Joystick_ControllerMapStage::OPEN,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        // Write the axis mappings
        NATIVE_INT_TYPE size = sizeof(axis_mappings);
        status = file.read(&axis_mappings, size);
        if (status != Os::File::Status::OP_OK || size != sizeof(axis_mappings))
        {
            log_WARNING_LO_ControllerMapLoadFailed(
                    filename, Joystick_ControllerMapStage::AXIS,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        // Write the button mappings
        size = sizeof(button_mappings);
        status = file.read(&button_mappings, size);
        if (status != Os::File::Status::OP_OK || size != sizeof(button_mappings))
        {
            log_WARNING_LO_ControllerMapLoadFailed(
                    filename, Joystick_ControllerMapStage::BUTTON,
                    status);
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        file.close();

        log_ACTIVITY_LO_LoadedControllerMapping(filename);
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    bool Joystick::Mapping::is_mapped() const
    {
        return channel >= 0 && channel < INAV_MSP_RC_CHANNEL;
    }

    Joystick::Mapping::Mapping()
            : channel(-1)
    {
    }

    U16 Joystick::AxisMapping::get_fc(I16 js_value) const
    {
        if (std::abs(js_value) <= deadzone)
        {
            return min_value;
        }

        // Interpolate between the min/max values
        F64 normalized = ((F64) js_value) / (1 << 15);
        return (U16) (normalized * (max_value - min_value) + min_value);
    }

    Joystick::AxisMapping::AxisMapping()
            : deadzone(0), min_value(0), max_value(0)
    {
    }

    void Joystick::AxisMapping::unmap()
    {
        channel = -1;
        deadzone = 0;
        min_value = 0;
        max_value = 0;
    }

    void Joystick::ButtonMapping::set_fc(I16 js_value, U16& fc) const
    {
        if (js_value && on_value >= 0)
        {
            fc = on_value;
        }
        else if (off_value >= 0)
        {
            fc = off_value;
        }
    }

    Joystick::ButtonMapping::ButtonMapping()
            : off_value(0), on_value(0)
    {
    }

    void Joystick::ButtonMapping::unmap()
    {
        channel = 0;
        off_value = 0;
        on_value = 0;
    }
}
