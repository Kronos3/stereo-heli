module Heli {

    # ----------------------------------------------------------------------
    # Defaults
    # ----------------------------------------------------------------------

    module Default {
        constant queueSize = 10
        constant stackSize = 128 * 1024
    }

    # ----------------------------------------------------------------------
    # Active component instances
    # ----------------------------------------------------------------------

    instance blockDrv: Drv.BlockDriver base id 100 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 140

    instance rg1Hz: Svc.ActiveRateGroup base id 200 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 120 \
    {

        phase Fpp.ToCpp.Phases.configObjects """
        NATIVE_INT_TYPE context[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        """

        phase Fpp.ToCpp.Phases.configComponents """
        rg1Hz.configure(
            ConfigObjects::rg1Hz::context,
            FW_NUM_ARRAY_ELEMENTS(ConfigObjects::rg1Hz::context)
        );
    """

    }

    instance rg5Hz: Svc.ActiveRateGroup base id 250 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 120 \
    {

        phase Fpp.ToCpp.Phases.configObjects """
        NATIVE_INT_TYPE context[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        """

        phase Fpp.ToCpp.Phases.configComponents """
        rg5Hz.configure(
            ConfigObjects::rg5Hz::context,
            FW_NUM_ARRAY_ELEMENTS(ConfigObjects::rg5Hz::context)
        );
    """

    }

    instance cmdDisp: Svc.CommandDispatcher base id 300 \
        queue size 20 \
        stack size Default.stackSize \
        priority 101

    instance fileManager: Svc.FileManager base id 500 \
        queue size 30 \
        stack size Default.stackSize \
        priority 100

    instance fileDownlink: Svc.FileDownlink base id 600 \
        queue size 30 \
        stack size Default.stackSize \
        priority 100 \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
            TIMEOUT = 1000,
            COOLDOWN = 1000,
            CYCLE_TIME = 1000,
            FILE_QUEUE_DEPTH = 10
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            fileDownlink.configure(
            ConfigConstants::fileDownlink::TIMEOUT,
            ConfigConstants::fileDownlink::COOLDOWN,
            ConfigConstants::fileDownlink::CYCLE_TIME,
            ConfigConstants::fileDownlink::FILE_QUEUE_DEPTH
        );
        """
    }

    instance eventLogger: Svc.ActiveLogger base id 700 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 98

    instance chanTlm: Svc.TlmChan base id 800 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 97

    instance prmDb: Svc.PrmDb base id 900 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 96 \
    {

        phase Fpp.ToCpp.Phases.instances """
        Svc::PrmDb prmDb(FW_OPTIONAL_NAME("prmDb"), "heli_prm.dat");
        """

        phase Fpp.ToCpp.Phases.readParameters """
        prmDb.readParamFile();
        """

    }

    # ----------------------------------------------------------------------
    # Passive component instances
    # ----------------------------------------------------------------------

    @ Communications driver. May be swapped with other comm drivers like UART
    @ Note: Here we have TCP reliable uplink and UDP (low latency) downlink
    instance comm: Drv.ByteStreamDriverModel base id 1000 \
        type "Drv::TcpClient" \
        at "../../Drv/TcpClient/TcpClient.hpp" \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
            PRIORITY = 100,
            STACK_SIZE = 128 * 1024
        };
        """

        phase Fpp.ToCpp.Phases.startTasks """
        // Initialize socket server if and only if there is a valid specification
        if (state.hostName != nullptr && state.portNumber != 0) {
            Os::TaskString name("ReceiveTask");
            // Uplink is configured for receive so a socket task is started
            comm.configure(state.hostName, state.portNumber);
            comm.startSocketTask(
                name,
                true,
                ConfigConstants::comm::PRIORITY,
                ConfigConstants::comm::STACK_SIZE
            );
        }
        """

        phase Fpp.ToCpp.Phases.freeThreads """
        comm.stopSocketTask();
        (void) comm.joinSocketTask(nullptr);
        """

    }

    instance downlink: Svc.Framer base id 1100 \
    {

        phase Fpp.ToCpp.Phases.configObjects """
        Svc::FprimeFraming framing;
        """

        phase Fpp.ToCpp.Phases.configComponents """
        downlink.setup(ConfigObjects::downlink::framing);
        """

    }


    instance uplink: Svc.Deframer base id 1150 \
    {
        phase Fpp.ToCpp.Phases.configObjects """
        Svc::FprimeDeframing deframing;
        """

        phase Fpp.ToCpp.Phases.configComponents """
        uplink.setup(ConfigObjects::uplink::deframing);
        """
    }


    instance staticMemory: Svc.StaticMemory base id 1200

    instance fatalAdapter: Svc.AssertFatalAdapter base id 1250

    instance fatalHandler: Svc.FatalHandler base id 1300

    instance systemTime: Svc.Time base id 1400 \
        type "Svc::LinuxTime" \
        at "../../Svc/LinuxTime/LinuxTime.hpp"

    instance rgDriver: Svc.RateGroupDriver base id 1500 \
    {

        phase Fpp.ToCpp.Phases.configObjects """
        NATIVE_INT_TYPE rgDivs[Svc::RateGroupDriver::DIVIDER_SIZE] = { 5, 1 };
        """

        phase Fpp.ToCpp.Phases.configComponents """
        rgDriver.configure(
            ConfigObjects::rgDriver::rgDivs,
            FW_NUM_ARRAY_ELEMENTS(ConfigObjects::rgDriver::rgDivs)
        );
        """

    }

    instance systemResources: Svc.SystemResources base id 1600

    instance fileUplink: Svc.FileUplink base id 1700 \
        queue size 30 \
        stack size Default.stackSize \
        priority 100


    instance fileUplinkBufferManager: Svc.BufferManager base id 0x4400 {
        phase Fpp.ToCpp.Phases.configConstants """
        enum {
            STORE_SIZE = 3000,
            QUEUE_SIZE = 30,
            MGR_ID = 200
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            Svc::BufferManager::BufferBins upBuffMgrBins;
            memset(&upBuffMgrBins, 0, sizeof(upBuffMgrBins));
            {
                using namespace ConfigConstants::fileUplinkBufferManager;
                upBuffMgrBins.bins[0].bufferSize = STORE_SIZE;
                upBuffMgrBins.bins[0].numBuffers = QUEUE_SIZE;
                fileUplinkBufferManager.setup(
                    MGR_ID,
                    0,
                    Allocation::mallocator,
                    upBuffMgrBins
                );
            }
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            fileUplinkBufferManager.cleanup();
        """
    }

    instance serialBufferManager: Svc.BufferManager base id 0x4500 {
        phase Fpp.ToCpp.Phases.configConstants """
        enum {
            STORE_SIZE = 6000,
            NUM_BUFFERS = 16,
            MGR_ID = 400
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            Svc::BufferManager::BufferBins serialBuffMgrBins;
            memset(&serialBuffMgrBins, 0, sizeof(serialBuffMgrBins));
            {
                using namespace ConfigConstants::serialBufferManager;
                serialBuffMgrBins.bins[0].bufferSize = STORE_SIZE;
                serialBuffMgrBins.bins[0].numBuffers = NUM_BUFFERS;
                serialBufferManager.setup(
                    MGR_ID,
                    0,
                    Allocation::mallocator,
                    serialBuffMgrBins
                );
            }
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            serialBufferManager.cleanup();
        """
    }

    instance cmdSeq: Svc.CmdSequencer base id 2000 \
            queue size Default.queueSize \
            stack size Default.stackSize \
            priority 100 \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
          BUFFER_SIZE = 5 * 1024
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            cmdSeq.allocateBuffer(
                0,
                Allocation::mallocator,
                ConfigConstants::cmdSeq::BUFFER_SIZE
            );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            cmdSeq.deallocateBuffer(Allocation::mallocator);
        """

    }

    instance cmdSeq2: Svc.CmdSequencer base id 2100 \
            queue size Default.queueSize \
            stack size Default.stackSize \
            priority 100 \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
          BUFFER_SIZE = 5 * 1024
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            cmdSeq2.allocateBuffer(
                0,
                Allocation::mallocator,
                ConfigConstants::cmdSeq2::BUFFER_SIZE
            );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            cmdSeq2.deallocateBuffer(Allocation::mallocator);
        """

    }

    instance cmdSeq3: Svc.CmdSequencer base id 2200 \
            queue size Default.queueSize \
            stack size Default.stackSize \
            priority 100 \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
          BUFFER_SIZE = 5 * 1024
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            cmdSeq3.allocateBuffer(
                0,
                Allocation::mallocator,
                ConfigConstants::cmdSeq3::BUFFER_SIZE
            );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            cmdSeq3.deallocateBuffer(Allocation::mallocator);
        """

    }

    instance cmdSeq4: Svc.CmdSequencer base id 2300 \
            queue size Default.queueSize \
            stack size Default.stackSize \
            priority 100 \
    {

        phase Fpp.ToCpp.Phases.configConstants """
        enum {
          BUFFER_SIZE = 5 * 1024
        };
        """

        phase Fpp.ToCpp.Phases.configComponents """
            cmdSeq4.allocateBuffer(
                0,
                Allocation::mallocator,
                ConfigConstants::cmdSeq4::BUFFER_SIZE
            );
        """

        phase Fpp.ToCpp.Phases.tearDownComponents """
            cmdSeq4.deallocateBuffer(Allocation::mallocator);
        """

    }

    instance cam: Cam base id 6000 \
    {
        phase Fpp.ToCpp.Phases.configComponents """
        cam.configure(/* (width,height) */ 640, 240,
                      /* left id, right id */ 0, 1,
                      /* left eye (rotation,vflip,hflip) */ 0, false, false,
                      /* right eye (rotation,vflip,hflip) */ 0, false, false);
        """

        phase Fpp.ToCpp.Phases.startTasks """
        Fw::String camTaskName = "CAM";
        cam.startStreamThread(camTaskName);
        """

        phase Fpp.ToCpp.Phases.stopTasks """
        cam.quitStreamThread();
        """
    }

    instance framePipe: FramePipe base id 6100

    instance videoStreamer: VideoStreamer base id 6200 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 100

    instance display: Display base id 6300

    instance vis: Vis base id 6400 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 100

    instance fc: Fc base id 6500 \
        queue size 50 \
        stack size Default.stackSize \
        priority 100

    instance sapp: Sapp base id 6600 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 100

    instance joystick: Joystick base id 6700

    instance serial0: Drv.LinuxSerialDriver base id 8000 \
      {

        phase Fpp.ToCpp.Phases.configComponents """
        {
          // Use serial 2 + 3 (PL011) that don't overlap with BT
          const bool status = serial0.open("/dev/ttyAMA1",
              Drv::LinuxSerialDriverComponentImpl::BAUD_460K,
              Drv::LinuxSerialDriverComponentImpl::NO_FLOW,
              Drv::LinuxSerialDriverComponentImpl::PARITY_NONE,
              true
          );
          if (!status) {
            Fw::Logger::logMsg("[ERROR] Could not open UART driver\\n");
            Init::status = false;
          }
        }
        """

        phase Fpp.ToCpp.Phases.startTasks """
        if (Init::status) {
          serial0.startReadThread();
        }
        else {
          Fw::Logger::logMsg("[ERROR] Initialization failed; not starting UART driver\\n");
        }
        """

        phase Fpp.ToCpp.Phases.stopTasks """
        serial0.quitReadThread();
        """

      }

    instance serial1: Drv.LinuxSerialDriver base id 8100 \
      {

        phase Fpp.ToCpp.Phases.configComponents """
        {
          // Use serial 2 + 3 (PL011) that don't overlap with BT
          const bool status = serial1.open("/dev/ttyAMA2",
              Drv::LinuxSerialDriverComponentImpl::BAUD_460K,
              Drv::LinuxSerialDriverComponentImpl::NO_FLOW,
              Drv::LinuxSerialDriverComponentImpl::PARITY_NONE,
              true
          );
          if (!status) {
            Fw::Logger::logMsg("[ERROR] Could not open UART driver\\n");
            Init::status = false;
          }
        }
        """

        phase Fpp.ToCpp.Phases.startTasks """
        if (Init::status) {
          serial1.startReadThread();
        }
        else {
          Fw::Logger::logMsg("[ERROR] Initialization failed; not starting UART driver\\n");
        }
        """

        phase Fpp.ToCpp.Phases.stopTasks """
        serial1.quitReadThread();
        """

      }

    instance cadre: Cadre base id 10001

}
