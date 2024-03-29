
#ifndef HELI_CAM_HPP
#define HELI_CAM_HPP

#include <CamCfg.hpp>

#include <Heli/Cam/CamComponentAc.hpp>
#include <Heli/Cam/CameraConfig.hpp>
#include <Heli/Cam/CamBuffer.hpp>
#include <Heli/Cam/core/completed_request.hpp>

#include <queue>
#include <mutex>

namespace Heli
{
    class LibcameraApp;
    class Cam : public CamComponentBase
    {
    public:
        explicit Cam(const char* compName);
        ~Cam() override;

        void init(NATIVE_INT_TYPE instance);

        void configure(I32 left_id, I32 right_id);

        void startStreamThread(const Fw::StringBase &name);
        void quitStreamThread();

    PRIVATE:
        void parametersLoaded() override;
        void parameterUpdated();

        void get_config(CameraConfig& config);
        bool frameGet_handler(NATIVE_INT_TYPE portNum, U32 frameId, CamFrame &left, CamFrame &right) override;
        void incdec_handler(NATIVE_INT_TYPE portNum, U32 frameId, const ReferenceCounter &dir) override;

        void STOP_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void START_cmdHandler(U32 opCode, U32 cmdSeq) override;

        void CONFIGURE_cmdHandler(U32 opCode, U32 cmdSeq,
                                  U32 width, U32 height,
                                  Heli::Cam_Rotation l_rot, Heli::Cam_Rotation r_rot,
                                  bool l_vflip, bool r_vflip,
                                  bool l_hflip, bool r_hflip) override;

        CamBuffer* get_buffer();

        static void streaming_thread_entry(void* this_);
        void streaming_thread();

    PRIVATE:
        void start();
        void stop();

        std::mutex m_buffer_mutex;
        CamBuffer m_buffers[CAMERA_BUFFER_N];

        bool m_configured;
        I32 m_left_id;
        I32 m_right_id;

        LibcameraApp* m_left;
        LibcameraApp* m_right;
        Os::Task m_task;

        U32 tlm_dropped;
        U32 tlm_captured;

        bool m_streaming;
    };
}

#endif //HELI_CAM_HPP
