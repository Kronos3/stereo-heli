//
// Created by tumbar on 3/31/22.
//

#ifndef STEREO_HELI_JOYSTICK_HPP
#define STEREO_HELI_JOYSTICK_HPP

#include <Heli/Joystick/JoystickComponentAc.hpp>

#include <Os/Mutex.hpp>

#include <Heli/Joystick/Joystick_AxisEnumAc.hpp>
#include <Heli/Joystick/Joystick_ButtonEnumAc.hpp>

namespace Heli
{
    class Joystick : public JoystickComponentBase
    {
    public:
        explicit Joystick(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:
        void STOP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
        void START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 js) override;

        void LOAD_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg &filename) override;
        void SAVE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg &filename) override;
        void MAP_AXIS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                 Heli::Joystick_Axis axis, I8 channel,
                                 U16 dead_zone, U16 min_value, U16 max_value) override;
        void MAP_BUTTON_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                   Heli::Joystick_Button button, I8 channel,
                                   I16 off_value, I16 on_value) override;

        enum JoystickEventType
        {
            BUTTON = 0x1,
            AXIS = 0x2,
            INIT = 0x80
        };

        struct JoystickEvent
        {
            /**
             * Timestamp in milliseconds of the event
             */
            U32 time;

            /**
             * The value associated with this joystick event.
             * For buttons this will be either 1 (down) or 0 (up).
             * For axes, this will range between
             * MIN_AXES_VALUE and MAX_AXES_VALUE.
             */
            I16 value;

            /**
             * Denotes if this event is for axis,
             * button or initialization
             */
            U8 type;

            /**
             * Axis/button number
             */
            U8 number;
        };

        static_assert(sizeof(JoystickEvent) == 8);

        static void main_loop(void* this_);
        void main_loop();

        void handle_event(const JoystickEvent& event);

        void setup_reply(FwOpcodeType opcode, U32 cmdSeq);
        void reply(const Fw::CmdResponse& response);

        void send_control();

    PRIVATE:
        Os::Mutex m_mutex;
        Os::Task m_task;

        U8 m_joystick;
        volatile bool is_running;

        bool cmd_waiting;
        FwOpcodeType m_opcode;
        U32 m_cmdSeq;

        I32 m_fd;

    PRIVATE:
        struct Mapping
        {
            I8 channel;
            Mapping();
            bool is_mapped() const;
        };

        struct AxisMapping : public Mapping
        {
            U16 deadzone;
            U16 min_value;
            U16 max_value;

            AxisMapping();
            U16 get_fc(I16 js_value) const;
            void unmap();
        };

        struct ButtonMapping : public Mapping
        {
            I16 off_value;
            I16 on_value;

            ButtonMapping();
            void set_fc(I16 js_value, U16& fc) const;
            void unmap();
        };

        MspMessage inav_msg;
        AxisMapping axis_mappings[Joystick_Axis::NUM_CONSTANTS];
        ButtonMapping button_mappings[Joystick_Button::NUM_CONSTANTS];

        U16 channels[INAV_MSP_RC_CHANNEL];
    };
}

#endif //STEREO_HELI_JOYSTICK_HPP
