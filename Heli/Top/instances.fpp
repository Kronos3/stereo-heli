module Heli {

    # ----------------------------------------------------------------------
    # Defaults
    # ----------------------------------------------------------------------

    module Default {
        constant queueSize = 10
        constant stackSize = 64 * 1024
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

    instance eventLogger: Svc.ActiveLogger base id 600 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 98

    instance chanTlm: Svc.TlmChan base id 700 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 97

    instance prmDb: Svc.PrmDb base id 800 \
        queue size Default.queueSize \
        stack size Default.stackSize \
        priority 96 \
    {

        phase Fpp.ToCpp.Phases.instances """
        Svc::PrmDb prmDb(FW_OPTIONAL_NAME("prmDb"), "PrmDb.dat");
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
            STACK_SIZE = Default::stackSize
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

    instance fatalAdapter: Svc.AssertFatalAdapter base id 1200

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

}
