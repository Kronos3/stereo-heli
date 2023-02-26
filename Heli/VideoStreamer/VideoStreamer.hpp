
#ifndef HELI_VIDEOSTREAMER_HPP
#define HELI_VIDEOSTREAMER_HPP

#include <Heli/VideoStreamer/VideoStreamerComponentAc.hpp>

#include <preview/preview.hpp>
#include <encoder/encoder.hpp>
#include <output/output.hpp>

#include <Fw/Types/String.hpp>

#include <vector>
#include <queue>

namespace Heli
{
    struct VideoStreamerImpl;
    class VideoStreamer : public VideoStreamerComponentBase
    {
    public:
        explicit VideoStreamer(const char* compName);
        ~VideoStreamer() override;

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

    PRIVATE:
        void preamble() override;

        void frame_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;
        void NETWORK_SEND_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& address, U16 portN) override;
        void DISPLAY_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, VideoStreamer_DisplayLocation where, CamSelect eye) override;
        void CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                const Fw::CmdStringArg &location,
                                Heli::CamSelect eye,
                                Heli::ImageEncoding encoding) override;

        void sched_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context) override;

    PRIVATE:
        void clean();

        bool is_capturing;
        struct {
            Fw::Time request_time;

            Fw::String location;
            Heli::CamSelect eye;
            Heli::ImageEncoding encoding;

            FwOpcodeType opCode;
            U32 cmdSeq;
        } m_capture;

        bool is_showing;
        CamFrame m_showing;
        VideoStreamer_DisplayLocation m_displaying;
        CamSelect m_eye;

        VideoStreamerImpl* m_impl;

        std::queue<U32> encoding_buffers;

        Fw::Time m_last_frame;          //!< Last sent frame for calculate frame rate
        U32 tlm_total_frames;
    };
}

#endif //HELI_VIDEOSTREAMER_HPP
