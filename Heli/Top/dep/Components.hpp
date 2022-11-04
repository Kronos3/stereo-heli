#ifndef RPI_COMPONENTS_H
#define RPI_COMPONENTS_H

// FPrime Core
#include "fprime/Svc/ActiveRateGroup/ActiveRateGroup.hpp"
#include "fprime/Svc/RateGroupDriver/RateGroupDriver.hpp"

#include "fprime/Svc/CmdDispatcher/CommandDispatcherImpl.hpp"
#include "fprime/Svc/CmdSequencer/CmdSequencerImpl.hpp"
//#include <Svc/Deframer/Deframer.hpp>
#include "fprime/Svc/Framer/FramerComponentImpl.hpp"

#include "fprime/Svc/ActiveLogger/ActiveLoggerImpl.hpp"
#include "fprime/Svc/TlmChan/TlmChanImpl.hpp"
#include "fprime/Svc/PrmDb/PrmDbImpl.hpp"

#include "fprime/Svc/FileManager/FileManager.hpp"
#include "fprime/Svc/StaticMemory/StaticMemoryComponentImpl.hpp"
//#include <Svc/FatalHandler/FatalHandlerComponentImpl.hpp>
#include "fprime/Svc/FileUplink/FileUplink.hpp"
#include "fprime/Svc/FileDownlink/FileDownlink.hpp"

#include "fprime/Drv/LinuxSerialDriver/LinuxSerialDriverComponentImpl.hpp"
#include "fprime/Drv/LinuxI2cDriver/LinuxI2cDriverComponentImpl.hpp"
//#include <Drv/LinuxSpiDriver/LinuxSpiDriverComponentImpl.hpp>
#include "fprime/Drv/TcpClient/TcpClientComponentImpl.hpp"
#include "fprime/Svc/LinuxTime/LinuxTimeImpl.hpp"
#include "fprime/Svc/LinuxTimer/LinuxTimerComponentImpl.hpp"

#include "Rpi/Cam/Cam.hpp"
#include "Rpi/VideoStreamer/VideoStreamer.hpp"
#include <Rpi/Mot/MotImpl.h>
#include <Rpi/Vis/VisImpl.h>
#include <Rpi/Nav/NavImpl.h>
#include "Rpi/FramePipe/FramePipeImpl.h"
#include "Rpi/Display/Display.hpp"

#include "fprime/Svc/BufferManager/BufferManagerComponentImpl.hpp"
#include "fprime/Svc/FramingProtocol/FprimeProtocol.hpp"
#include "fprime/Fw/Types/MallocAllocator.hpp"
#include "fprime/Os/Log.hpp"

class Kernel
{
    Os::Log os_logger;

    Svc::RateGroupDriver rgDriver;
    Svc::ActiveRateGroup rg1hz;
    Svc::ActiveRateGroup rg10hz;
    Svc::ActiveRateGroup rg20hz;
    Svc::ActiveRateGroup rg50hz;

    Fw::MallocAllocator mallocAllocator;
    Svc::CmdSequencerComponentImpl cmdSeq;
    Svc::CommandDispatcherImpl cmdDisp;

    Svc::ActiveLoggerImpl eventLogger;
    Svc::TlmChanImpl chanTlm;
    Svc::PrmDbImpl prmDb;

    Svc::Deframer uplink;
    Svc::FramerComponentImpl downlink;
    Svc::FileUplink fileUplink;
    Svc::BufferManagerComponentImpl fileUplinkBufferManager;
    Svc::FileDownlink fileDownlink;

    Svc::FileManager fileManager;
    Svc::StaticMemoryComponentImpl staticMemory;
//    Svc::FatalHandlerComponentImpl fatalHandler;

    Svc::LinuxTimerComponentImpl linuxTimer;
    Svc::LinuxTimeImpl systemTime;
    Drv::LinuxSerialDriverComponentImpl serialDriver;
    Drv::LinuxI2cDriverComponentImpl i2cDriver;
//    Drv::LinuxSpiDriverComponentImpl spiDriver;
    Drv::TcpClientComponentImpl comm;

    Rpi::CamImpl cam;
    Rpi::VideoStreamerImpl videoStreamer;
    Rpi::MotImpl mot;
    Rpi::VisImpl vis;
    Rpi::NavImpl nav;
    Rpi::FramePipeImpl framePipe;
    Rpi::DisplayImpl display;

    Svc::FprimeDeframing deframing;
    Svc::FprimeFraming framing;
    Fw::MallocAllocator mallocator;

    // FSW entry points
    void prv_init();
    void prv_start();
    void prv_reg_commands();
    void prv_loadParameters();

    void setRpiRTLinuxIds();
    void constructRpiRTLinuxArchitecture();

public:
    Kernel();

    void start();
    void run();
    void exit();
};

#endif //RPI_COMPONENTS_H
