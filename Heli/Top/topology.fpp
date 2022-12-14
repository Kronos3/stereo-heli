module Heli {

    enum Port_RateGroups {
        rg1Hz
        rg5Hz
    }

    enum Ports_StaticMemory {
      downlink
      uplink
    }

    topology Heli {
        # Core components
        instance systemTime
        instance eventLogger
        instance cmdDisp
        instance chanTlm
        instance prmDb
        instance comm
        instance downlink
        instance uplink
        instance fileDownlink
        instance fileManager
        instance fileUplink
        instance fileUplinkBufferManager
        instance cmdSeq
        instance cmdSeq2
        instance cmdSeq3
        instance cmdSeq4
        instance staticMemory

        # Rate groups
        instance blockDrv
        instance rgDriver
        instance rg1Hz
        instance rg5Hz

        # Rpi components
        instance cam
        instance framePipe
        instance display
        instance videoStreamer
        instance vis

        # ---------------------------------
        # Pattern graph connections
        # ---------------------------------

        command connections instance cmdDisp
        event connections instance eventLogger
        time connections instance systemTime

        param connections instance prmDb
        telemetry connections instance chanTlm

        # ---------------------------------
        # Core graph connections
        # ---------------------------------

        connections RateGroups {
            blockDrv.CycleOut -> rgDriver.CycleIn

            # Rate group 1Hz
            rgDriver.CycleOut[Port_RateGroups.rg1Hz] -> rg1Hz.CycleIn
            rgDriver.CycleOut[Port_RateGroups.rg5Hz] -> rg5Hz.CycleIn
            rg1Hz.RateGroupMemberOut[0] -> chanTlm.Run
            rg1Hz.RateGroupMemberOut[1] -> cmdSeq.schedIn
            rg1Hz.RateGroupMemberOut[2] -> cmdSeq2.schedIn
            rg1Hz.RateGroupMemberOut[3] -> cmdSeq3.schedIn
            rg1Hz.RateGroupMemberOut[4] -> cmdSeq4.schedIn
        }

        connections Sequencer {
            cmdSeq.comCmdOut -> cmdDisp.seqCmdBuff[0]
            cmdSeq2.comCmdOut -> cmdDisp.seqCmdBuff[1]
            cmdSeq3.comCmdOut -> cmdDisp.seqCmdBuff[2]
            cmdSeq4.comCmdOut -> cmdDisp.seqCmdBuff[3]
            cmdDisp.seqCmdStatus[0] -> cmdSeq.cmdResponseIn
            cmdDisp.seqCmdStatus[1] -> cmdSeq2.cmdResponseIn
            cmdDisp.seqCmdStatus[2] -> cmdSeq3.cmdResponseIn
            cmdDisp.seqCmdStatus[3] -> cmdSeq4.cmdResponseIn
        }

        # --------------------------------
        # HELI Graph Connections
        # --------------------------------

        connections Camera {
            # Frame buffer control
            videoStreamer.incdec -> cam.incdec
            framePipe.incdec -> cam.incdec

            videoStreamer.frameGet -> cam.frameGet
            vis.frameGet -> cam.frameGet

            # Video Streamer pipeline
            cam.frame -> framePipe.camFrame

            # Frame pipeline
            framePipe.frame[0] -> videoStreamer.frame
            framePipe.frame[1] -> vis.frame
            vis.frameOut -> framePipe.frameIn
        }

        # --------------------------------
        # Comm Connections
        # --------------------------------

        connections Downlink {

              chanTlm.PktSend -> downlink.comIn
              eventLogger.PktSend -> downlink.comIn
              fileDownlink.bufferSendOut -> downlink.bufferIn

              downlink.framedAllocate -> staticMemory.bufferAllocate[Ports_StaticMemory.downlink]
              downlink.framedOut -> comm.send
              downlink.bufferDeallocate -> fileDownlink.bufferReturn

              comm.deallocate -> staticMemory.bufferDeallocate[Ports_StaticMemory.downlink]

        }

        connections Uplink {

              comm.allocate -> staticMemory.bufferAllocate[Ports_StaticMemory.uplink]
              comm.$recv -> uplink.framedIn
              uplink.framedDeallocate -> staticMemory.bufferDeallocate[Ports_StaticMemory.uplink]

              uplink.comOut -> cmdDisp.seqCmdBuff
              cmdDisp.seqCmdStatus -> uplink.cmdResponseIn

              uplink.bufferAllocate -> fileUplinkBufferManager.bufferGetCallee
              uplink.bufferOut -> fileUplink.bufferSendIn
              uplink.bufferDeallocate -> fileUplinkBufferManager.bufferSendIn
              fileUplink.bufferSendOut -> fileUplinkBufferManager.bufferSendIn

        }

        # --------------------------------
        # Driver Connections
        # --------------------------------

        connections Drivers {

        }
    }

}