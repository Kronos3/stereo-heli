
#include <Rpi/Display/Display.hpp>
#include <Fw/Logger/Logger.hpp>

namespace Rpi
{
    Display::Display(const char* name)
    : DisplayComponentBase(name), Oled()
    {
    }

    void Display::init(NATIVE_INT_TYPE instance)
    {
        DisplayComponentBase::init(instance);
    }

    void Display::oled_init()
    {
        Oled::init();
    }

    void Display::i2c_write(U8* data, U32 len)
    {
        Fw::Buffer i2c_buffer(data, len);
        Drv::I2cStatus status = i2c_out(0, I2C_ADDRESS, i2c_buffer);

        if (status != Drv::I2cStatus::I2C_OK)
        {
            Fw::Logger::logMsg("Failed to write i2c to display: %d\n",
                               status.e);
        }
    }

    void Display::write(U32 line_number, const char* text)
    {
        FW_ASSERT(text);
        FW_ASSERT(line_number < 4, line_number);

        m_access.lock();

        line(line_number, 0, text);
        draw();

        m_access.unLock();
    }

    void Display::WRITE_cmdHandler(U32 opCode, U32 cmdSeq, U8 lineIndex, const Fw::CmdStringArg &lineText)
    {
        if (lineIndex >= LINE_MAX_N)
        {
            cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
            return;
        }

        write(lineIndex, lineText.toChar());
        cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}
