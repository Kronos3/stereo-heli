
#ifndef HELI_VIDEOSTREAMER_HPP
#define HELI_VIDEOSTREAMER_HPP

#include <Rpi/VideoStreamer/VideoStreamerComponentAc.hpp>

#include <preview/preview.hpp>
#include <encoder/encoder.hpp>
#include <output/output.hpp>

#include <Fw/Types/String.hpp>

#include <vector>
#include <queue>

namespace Rpi
{
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
        void OPEN_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg& address, U16 portN) override;
        void DISPLAY_cmdHandler(U32 opCode, U32 cmdSeq, VideoStreamer_DisplayLocation where) override;
//        void CAPTURE_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg &destination) override;

    PRIVATE:
        void clean();

        bool is_showing;
        CamFrame m_showing;
        VideoStreamer_DisplayLocation m_displaying;

        Preview* m_preview;

        std::queue<U32> encoding_buffers;
        Encoder* m_encoder;
        Output* m_net;

        struct {
            bool requesting;
            Fw::String destination;
            U32 opcode;
            U32 cmdSeq;
        } m_capture;

        Fw::Time m_last_frame;          //!< Last sent frame for calculate frame rate
        U32 tlm_packets_sent;
        U32 tlm_total_frames;
    };
}

#endif //HELI_VIDEOSTREAMER_HPP
