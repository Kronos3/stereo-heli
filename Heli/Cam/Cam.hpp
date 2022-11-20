
#ifndef HELI_CAM_HPP
#define HELI_CAM_HPP

#include <Heli/Cam/CamComponentAc.hpp>
#include "CamCfg.h"
#include "CameraConfig.hpp"
#include "CamBuffer.hpp"
#include <core/completed_request.hpp>
#include <queue>
#include <mutex>

namespace Rpi
{
    class LibcameraApp;
    class Cam : public CamComponentBase
    {
//        friend LibcameraApp;

    public:
        explicit Cam(const char* compName);
        ~Cam() override;

        void init(NATIVE_INT_TYPE queueDepth, NATIVE_INT_TYPE instance);

        void configure(I32 videoWidth, I32 videoHeight,
                       I32 stillWidth, I32 stillHeight,
                       I32 rotation,
                       bool vflip, bool hflip);

        void startStreamThread(const Fw::StringBase &name);
        void quitStreamThread();

    PRIVATE:
        void parametersLoaded() override;
        void parameterUpdated();

        void get_config(CameraConfig& config);
        bool frameGet_handler(NATIVE_INT_TYPE portNum, U32 frameId, Rpi::CamFrame &frame) override;
        void incref_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;
        void decref_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;

        void CAPTURE_cmdHandler(U32 opCode, U32 cmdSeq,
                                Rpi::Cam_CamSelect cam_select,
                                const Fw::CmdStringArg &left_dest,
                                const Fw::CmdStringArg &right_dest) override;
        void STOP_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void START_cmdHandler(U32 opCode, U32 cmdSeq) override;

        CamBuffer* get_buffer();

        static void streaming_thread_entry(void* this_);
        void streaming_thread();

    PRIVATE:
        void start();
        void stop();
        void capture(U32 opcode, U32 cmdSeq);
        void finishCapture();

        std::mutex m_buffer_mutex;
        CamBuffer m_buffers[CAMERA_BUFFER_N];

        LibcameraApp* m_camera;
        Os::Task m_task;

        U32 tlm_dropped;
        U32 tlm_captured;

        U32 m_cmdSeq;
        U32 m_opcode;

        bool m_capturing;
        bool m_streaming;
    };
}

#endif //HELI_CAM_HPP
