#include "Components.hpp"
#include "Cfg/VoCarCfg.h"
#include <cstdlib>
#include <cerrno>

extern "C"
{
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
}

#define TIMER_HZ (100)

enum
{
    QUEUE_DEPTH = 32,
    FILE_UPLINK_QUEUE_DEPTH = 4096
};

enum
{
    UPLINK_BUFFER_STORE_SIZE = 4096,
    UPLINK_BUFFER_QUEUE_SIZE = 4096,
    UPLINK_BUFFER_MGR_ID = 200
};

static NATIVE_INT_TYPE rgDivs[Svc::RateGroupDriverImpl::DIVIDER_SIZE] = {
        TIMER_HZ / 1,
        TIMER_HZ / 10,
        TIMER_HZ / 20,
//            TIMER_HZ / 50,
};

U32 contexts[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Kernel::Kernel()
        :
        os_logger(),
        rgDriver("RGDRV"),
        rg1hz("RG1HZ", contexts, FW_NUM_ARRAY_ELEMENTS(contexts)),
        rg10hz("RG10HZ", contexts, FW_NUM_ARRAY_ELEMENTS(contexts)),
        rg20hz("RG20HZ", contexts, FW_NUM_ARRAY_ELEMENTS(contexts)),
        rg50hz("RG50HZ", contexts, FW_NUM_ARRAY_ELEMENTS(contexts)),
        cmdSeq("SEQ"),
        cmdDisp("DISP"),

        eventLogger("LOG"),
        chanTlm("TLM"),
        prmDb("PRM", "/fsw/data/prm.dat"),

        uplink("UPLINK"),
        downlink("DOWNLINK"),
        fileUplink("fileUplink"),
        fileUplinkBufferManager("fileUplinkBufferManager"),
        fileDownlink("fileDownlink"),

        fileManager("FILE_MGR"),
        staticMemory("STATIC_MEM"),
        //fatalHandler("FATAL_HANDLER"),

        linuxTimer("LINUX_TIMER"),
        systemTime("TIME"),
        serialDriver("SERIAL"),
        i2cDriver("I2C"),
        //spiDriver("SPI"),
        comm("COMM"),

        cam("CAM"),
        videoStreamer("VIDEO_STREAMER"),
        mot("MOT"),
        vis("VIS"),
        nav("NAV"),
        framePipe("F-PIPE"),
        display("DISPLAY")
{
}

void Kernel::prv_init()
{
    linuxTimer.init(0);

    rgDriver.init();
    rg1hz.init(QUEUE_DEPTH, 0);
    rg10hz.init(QUEUE_DEPTH, 1);
    rg20hz.init(QUEUE_DEPTH, 1);
    rg50hz.init(QUEUE_DEPTH, 3);

    comm.init(0);

    cmdSeq.init(QUEUE_DEPTH, 0);
    cmdSeq.allocateBuffer(0, mallocAllocator, 500 * 1024);
    cmdDisp.init(QUEUE_DEPTH, 0);

    eventLogger.init(QUEUE_DEPTH, 0);
    chanTlm.init(QUEUE_DEPTH, 0);
    prmDb.init(QUEUE_DEPTH, 0);

    staticMemory.init(0);
    uplink.init(0);
    downlink.init(0);

    downlink.setup(framing);
    uplink.setup(deframing);
    fileUplink.init(FILE_UPLINK_QUEUE_DEPTH, 0);
    fileUplinkBufferManager.init(0);

    // set up BufferManager instances
    Svc::BufferManagerComponentImpl::BufferBins upBuffMgrBins;
    memset(&upBuffMgrBins, 0, sizeof(upBuffMgrBins));
    upBuffMgrBins.bins[0].bufferSize = UPLINK_BUFFER_STORE_SIZE;
    upBuffMgrBins.bins[0].numBuffers = UPLINK_BUFFER_QUEUE_SIZE;
    fileUplinkBufferManager.setup(UPLINK_BUFFER_MGR_ID, 0, mallocator, upBuffMgrBins);

    fileDownlink.configure(1000, 200, 100, 10);
    fileDownlink.init(QUEUE_DEPTH, 0);

    fileManager.init(QUEUE_DEPTH, 0);
//        fatalHandler.init(0);

    systemTime.init(0);
    serialDriver.init(0);

    serialDriver.open("/dev/serial1",
                      Drv::LinuxSerialDriverComponentImpl::BAUD_115K,
                      Drv::LinuxSerialDriverComponentImpl::HW_FLOW,
                      Drv::LinuxSerialDriverComponentImpl::PARITY_NONE,
                      false);

    i2cDriver.init(0);
    bool opened = i2cDriver.open("/dev/i2c-1");
    FW_ASSERT(opened && "I2C failed to open", errno);

//        spiDriver.init(0);

    cam.init(0,
             CAMERA_RAW_WIDTH, CAMERA_RAW_HEIGHT,
             false, false, false);
    videoStreamer.init(QUEUE_DEPTH, 0);

    mot.init(0);
    vis.init(QUEUE_DEPTH, 0);
    nav.init(QUEUE_DEPTH, 0);
    framePipe.init(0);
    display.init(0);
}

void Kernel::prv_start()
{
    // Print the Ip address of the Raspberry Pi on the display
    display.oled_init();
    {
        struct ifaddrs* addrs = nullptr;
        struct ifaddrs* tmp;
        getifaddrs(&addrs);
        tmp = addrs;

        // Find the IP address of the first non-home interface
        // Print this IP to the first line on the OLED display
        while (tmp)
        {
            if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
            {
                auto* pAddr = (struct sockaddr_in *) tmp->ifa_addr;
                const char* addr_str = inet_ntoa(pAddr->sin_addr);
                if (strcmp(addr_str, "127.0.0.1") != 0)
                {
                    // This is the address we want
                    display.write(0, addr_str);
                    break;
                }
            }

            tmp = tmp->ifa_next;
        }

        freeifaddrs(addrs);
    }

    rg1hz.start();
    rg10hz.start();
    rg20hz.start();
    rg50hz.start();

    eventLogger.start();
    chanTlm.start();
    prmDb.start();

    fileUplink.start();
    fileManager.start();
    fileDownlink.start(0);

//        serialDriver.startReadThread();

    cmdSeq.start();
    cmdDisp.start();

    Fw::String s("cam");
    cam.startStreamThread(s);
    videoStreamer.start(19);
    vis.start();
    nav.start();

    // Always start this last (or first, but not in the middle)
    Os::File gds_cfg;
    Os::File::Status status = gds_cfg.open("/fsw/cfg/gds.cfg", Os::File::OPEN_READ);
    if (status != Os::File::OP_OK)
    {
        display.write(1, "No GDS!");
        Fw::Logger::logMsg("Failed to load GDS configuration /fsw/cfg/gds.cfg\n");
    }
    else
    {

        static char gds_hostname_port[32];
        NATIVE_INT_TYPE size = sizeof(gds_hostname_port);
        gds_cfg.read(gds_hostname_port, size);
        gds_hostname_port[size] = 0;

        gds_cfg.close();

        char* split = strchr(gds_hostname_port, ':');
        if (!split)
        {
            Fw::Logger::logMsg("Failed to parse GDS configuration, missing ':'\n");
        }
        else
        {
            *split = 0;
            U16 port = strtoul(split + 1, nullptr, 10);

            display.write(1, gds_hostname_port);
            Fw::Logger::logMsg("Connecting to GDS on %s:%d\n", (POINTER_CAST) gds_hostname_port, port);

            Fw::String comm_name = "comm";
            comm.configure(gds_hostname_port, port);
            comm.startSocketTask(comm_name);
            Os::Task::delay(1000);
        }
    }
}

void Kernel::prv_reg_commands()
{
    cmdSeq.regCommands();
    cmdDisp.regCommands();
    eventLogger.regCommands();
    prmDb.regCommands();
    cam.regCommands();
    videoStreamer.regCommands();
    mot.regCommands();
    vis.regCommands();
    nav.regCommands();
    display.regCommands();
    fileManager.regCommands();
    fileDownlink.regCommands();
}

void Kernel::prv_loadParameters()
{
//        mot.loadParameters();
    videoStreamer.loadParameters();
    cam.loadParameters();
    vis.loadParameters();
    nav.loadParameters();
}

void Kernel::start()
{
    Fw::Logger::logMsg("Initializing components\n");
    prv_init();

    Fw::Logger::logMsg("Initializing port connections\n");
    constructRpiRTLinuxArchitecture();

    Fw::Logger::logMsg("Starting active tasks\n");
    prv_start();

    Fw::Logger::logMsg("Registering commands\n");
    prv_reg_commands();

    Fw::Logger::logMsg("Load parameter database\n");
    prmDb.readParamFile();
    prv_loadParameters();

    Fw::Logger::logMsg("Boot complete\n");
}

void Kernel::run()
{
    // Run the scheduled tasks
    linuxTimer.startTimer(1000 / TIMER_HZ);
}

void Kernel::exit()
{
    linuxTimer.quit();

    rg1hz.exit();
    rg10hz.exit();
    rg20hz.exit();
    rg50hz.exit();

    eventLogger.exit();
    chanTlm.exit();
    prmDb.exit();

    fileUplink.exit();
    fileDownlink.exit();
    fileManager.exit();
    cmdSeq.exit();
    cmdSeq.deallocateBuffer(mallocAllocator);
    cmdDisp.exit();

    cam.quitStreamThread();
    videoStreamer.exit();
    vis.exit();
    nav.exit();

//        serialDriver.quitReadThread();

    rg1hz.join(nullptr);
    rg10hz.join(nullptr);
    rg20hz.join(nullptr);
    rg50hz.join(nullptr);

    eventLogger.join(nullptr);
    chanTlm.join(nullptr);
    prmDb.join(nullptr);

    fileUplink.join(nullptr);
    fileDownlink.join(nullptr);
    fileManager.join(nullptr);
    cmdSeq.join(nullptr);
    cmdDisp.join(nullptr);

    videoStreamer.join(nullptr);
    vis.join(nullptr);
    nav.join(nullptr);

    comm.stopSocketTask();
    comm.joinSocketTask(nullptr);
    fileUplinkBufferManager.cleanup();
}
