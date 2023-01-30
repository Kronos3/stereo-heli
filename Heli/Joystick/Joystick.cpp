//
// Created by tumbar on 1/13/23.
//

#include <fcntl.h>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <poll.h>
#include "Joystick.hpp"
#include "File.hpp"

namespace Heli
{
    Joystick::Joystick(const char* compName)
            : JoystickComponentBase(compName),
              m_joystick(0), is_running(false), cmd_waiting(false),
              m_opcode(0), m_cmdSeq(0), m_fd(-1), channels{},
              inav_msg(Fc_MspMessageId::MSP_SET_RAW_RC),
              m_failsafe_arr(0),
              m_failsafe_ticks(0),
              m_inav_msg_last_checksum(0),
              m_inav_msg_last_checksum_valid(false)
    {
        inav_msg.set_flags(/* Tell iNAV not to reply */ 1);
    }

    void Joystick::init(NATIVE_INT_TYPE instance)
    {
        JoystickComponentBase::init(instance);
    }

    void Joystick::STOP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq)
    {
        setup_reply(opCode, cmdSeq);

        m_mutex.lock();
        if (!is_running)
        {
            log_WARNING_LO_JoystickNotRunning();
            m_mutex.unlock();
            reply(Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        is_running = false;
        m_mutex.unlock();
    }

    void Joystick::START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 js,
                                    U8 trigger_ms,
                                    U32 delay_s,
                                    U32 failsafe_ms)
    {
        setup_reply(opCode, cmdSeq);

        m_mutex.lock();
        if (is_running)
        {
            log_WARNING_LO_JoystickAlreadyRunning();
            m_mutex.unlock();
            reply(Fw::CmdResponse::BUSY);
            return;
        }

        m_joystick = js;
        m_tim_interval = Fw::Time(0, trigger_ms * 1000);
        m_tim_value = Fw::Time(delay_s, 0);
        m_task = Os::Task();

        // Number of timer trigger ticks before we NEED to send a packet
        // This is basically a keep alive packet controlled by a watchdog
        // Normally if there is no RX during this time, the failsafe would be
        // triggered.
        // TODO(tumbar) Does this actually apply for MSP, do we even care?
        m_failsafe_arr = (failsafe_ms - 2 * trigger_ms) / trigger_ms;
        m_failsafe_ticks = m_failsafe_arr;

        m_inav_msg_last_checksum_valid = false;
        m_inav_msg_last_checksum = 0;

        m_mutex.unlock();

        Fw::String task_name = "JoystickListener";
        m_task.start(task_name, Joystick::main_loop, this);
    }

    void Joystick::main_loop()
    {
        Fw::String path;
        path.format("/dev/input/js%d", m_joystick);

        // Blocking call
        m_mutex.lock();
        m_fd = ::open(path.toChar(), O_RDONLY);
        m_mutex.unlock();

        if (m_fd < 0)
        {
            log_WARNING_HI_ControllerOpenFailed(path, errno);
            reply(Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        log_ACTIVITY_LO_JoystickStarting(m_joystick);

        // Reply to the start
        reply(Fw::CmdResponse::OK);

        pollfd fd_poll = {
                .fd = m_fd,
                .events = POLLIN,
                .revents = 0
        };

        startTimer_out(
                0,
                m_tim_interval,
                m_tim_value
        );

        JoystickEvent event{};
        bool error = false;
        is_running = true;
        while (is_running)
        {
            fd_poll.revents = 0;
            I32 rc = poll(&fd_poll, 1, 500);

            // Poll ran into an error OR poll() was file and the FD ran into error
            if (rc < 0 || (rc == 1 && !(fd_poll.revents & POLLIN)))
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
        stopTimer_out(0);

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

            m_mutex_control.lock();
            mapping.set_fc(event.value, channels[mapping.channel.e]);
            m_mutex_control.unlock();
        }
        else if (event.type & AXIS)
        {
            if (event.number >= FW_NUM_ARRAY_ELEMENTS(axis_mappings))
            {
                log_WARNING_HI_InvalidAxisNumber(event.number);
                return;
            }

            // Not const in case of axis differential mode
            AxisMapping &mapping = axis_mappings[event.number];
            if (!mapping.is_mapped())
            {
                // Ignore unmapped events
                return;
            }

            m_mutex_control.lock();
            mapping.set_fc(event.value, channels[mapping.channel.e]);
            m_mutex_control.unlock();
        }
    }

    void Joystick::MAP_AXIS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                       Heli::Joystick_Axis axis,
                                       Joystick_AETRChannel channel,
                                       U16 dead_zone,
                                       U16 min_value, U16 max_value,
                                       bool inverted)
    {
        AxisMapping &mapping = axis_mappings[axis.e];
        if (channel == Joystick_AETRChannel::UNMAPPED)
        {
            if (axis_mappings[axis.e].is_mapped())
            {
                mapping.unmap();
                log_ACTIVITY_LO_AxisUnmapped(axis);
            }
        }
        else
        {
            mapping.type = Joystick_AxisMapType::DIRECT;
            mapping.inverted = inverted;
            mapping.channel = channel;
            mapping.deadzone = dead_zone;
            mapping.min_value = min_value;
            mapping.max_value = max_value;
            mapping.delta_scale = 0.0;
            mapping.delta_offset = 0.0;

            log_ACTIVITY_LO_AxisMapped(axis, channel);
        }

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void Joystick::MAP_AXIS_DERIVATIVE_cmdHandler(
            FwOpcodeType opCode, U32 cmdSeq,
            Heli::Joystick_Axis axis,
            Heli::Joystick_AETRChannel channel,
            U16 dead_zone, U16 min_value, U16 max_value,
            F32 delta_scale, F32 delta_offset)
    {
        AxisMapping &mapping = axis_mappings[axis.e];
        if (channel == Joystick_AETRChannel::UNMAPPED)
        {
            if (axis_mappings[axis.e].is_mapped())
            {
                mapping.unmap();
                log_ACTIVITY_LO_AxisUnmapped(axis);
            }
        }
        else
        {
            mapping.type = Joystick_AxisMapType::DERIVATIVE;
            mapping.inverted = false;
            mapping.channel = channel;
            mapping.deadzone = dead_zone;
            mapping.min_value = min_value;
            mapping.max_value = max_value;
            mapping.delta_scale = delta_scale;
            mapping.delta_offset = delta_offset;
            log_ACTIVITY_LO_AxisMapped(axis, channel);
        }

        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }

    void
    Joystick::MAP_BUTTON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                    Heli::Joystick_Button button,
                                    Joystick_AETRChannel channel,
                                    Joystick_ButtonMapType type, I16 off_value, I16 on_value)
    {
        ButtonMapping &mapping = button_mappings[button.e];
        if (channel == Joystick_AETRChannel::UNMAPPED)
        {
            if (button_mappings[button.e].is_mapped())
            {
                mapping.unmap();
                log_ACTIVITY_LO_ButtonUnmapped(button);
            }
        }
        else
        {
            mapping.type = type;
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

    void Joystick::LOAD_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg &filename)
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

    void Joystick::sendControl_handler(NATIVE_INT_TYPE portNum, Svc::TimerVal &cycleStart)
    {
        for (const auto& iter : axis_mappings)
        {
            iter.service(channels);
        }

        m_mutex_control.lock();
        inav_msg.set_payload(reinterpret_cast<const U8*>(&channels), sizeof(channels));
        m_mutex_control.unlock();

        U8 current_checksum = inav_msg.get_checksum();

        // Don't send if nothing change on the controller
        if (m_inav_msg_last_checksum_valid &&
            current_checksum == m_inav_msg_last_checksum &&
            m_failsafe_ticks)
        {
            m_failsafe_ticks--;
            return;
        }

        // Stroke the dog
        m_failsafe_ticks = m_failsafe_arr;

        m_inav_msg_last_checksum = current_checksum;
        m_inav_msg_last_checksum_valid = true;

        fcMsg_out(0, inav_msg, 0, Fc_ReplyAction::NO_REPLY);
    }

    bool Joystick::Mapping::is_mapped() const
    {
        return channel != Joystick_AETRChannel::UNMAPPED;
    }

    Joystick::Mapping::Mapping()
            : channel(Joystick_AETRChannel::UNMAPPED)
    {
    }

    void Joystick::AxisMapping::set_fc(I16 js_value, U16 &fc)
    {
        if (std::abs(js_value) <= deadzone)
        {
            js_value = 0;
        }

        F64 normalized = (js_value / (32767.0 * 2)) + 0.5;

        if (inverted)
        {
            normalized = 1.0 - normalized;
        }

        normalized = FW_MAX(FW_MIN(normalized, 1.0), 0);

        switch (type.e)
        {
            case Joystick_AxisMapType::DIRECT:
                fc = static_cast<U16>(normalized * (max_value - min_value) + min_value);
                break;
            case Joystick_AxisMapType::DERIVATIVE:
                // Don't set using event based control
                // Use periodic control
                last_control = static_cast<F32>((normalized + delta_offset) * delta_scale);
                break;
        }
    }

    Joystick::AxisMapping::AxisMapping()
            : deadzone(0), inverted(false),
            min_value(0), max_value(0),
            delta_scale(0.0), delta_offset(0.0),
            last_control(0)
    {
    }

    void Joystick::AxisMapping::unmap()
    {
        channel = Joystick_AETRChannel::UNMAPPED;
        inverted = false;
        deadzone = 0;
        min_value = 0;
        max_value = 0;
        delta_scale = 0.0;
        delta_offset = 0.0;
        last_control = 0;
    }

    void Joystick::AxisMapping::service(U16* channels_) const
    {
        if (!is_mapped()) return;
        if (type != Joystick_AxisMapType::DERIVATIVE) return;

        channels_[channel.e] = FW_MAX(FW_MIN(channels_[channel.e] + last_control, max_value), min_value);
    }

    void Joystick::ButtonMapping::set_fc(I16 js_value, U16 &fc) const
    {
        if (type == Joystick_ButtonMapType::HOLD)
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
        else // Toggle
        {
            if (js_value && fc == on_value)
            {
                // Turn off
                fc = off_value;
            }
            else if (js_value)
            {
                // Turn on
                fc = on_value;
            }
        }
    }

    Joystick::ButtonMapping::ButtonMapping()
            : off_value(0), on_value(0)
    {
    }

    void Joystick::ButtonMapping::unmap()
    {
        channel = Joystick_AETRChannel::UNMAPPED;
        type = Joystick_ButtonMapType::HOLD;
        channel = 0;
        off_value = 0;
        on_value = 0;
    }
}
