//
// Created by tumbar on 11/20/22.
//

#ifndef STEREO_HELI_VIS_HPP
#define STEREO_HELI_VIS_HPP

#include <Heli/Vis/VisComponentAc.hpp>
#include <Heli/Vis/VisStage.hpp>

#include <vector>

namespace Heli
{
    class Vis : public VisComponentBase
    {
        friend VisStage;
        friend RectifyStage;
        friend StereoStage;
        friend DepthStage;
    public:
        explicit Vis(const char* componentName);

        void init(
                NATIVE_INT_TYPE queueDepth, /*!< The queue depth*/
                NATIVE_INT_TYPE instance = 0 /*!< The instance number*/
        );

    PRIVATE:
        void frame_handler(
                NATIVE_INT_TYPE portNum, /*!< The port number*/
                U32 frameId
        ) override;

        void CLEAR_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void SCALE_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, Heli::Vis_Interpolation interp) override;
        void RECTIFY_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void STEREO_cmdHandler(U32 opCode, U32 cmdSeq,
                               Vis_StereoAlgorithm algorithm) override;
        void DEPTH_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void COLORMAP_cmdHandler(U32 opCode, U32 cmdSeq, Vis_ColorMap colormap, CamSelect select) override;

        void MODEL_L_K_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, F32 cx, F32 cy) override;
        void MODEL_R_K_cmdHandler(U32 opCode, U32 cmdSeq, F32 fx, F32 fy, F32 cx, F32 cy) override;
        void MODEL_L_D_cmdHandler(U32 opCode, U32 cmdSeq, F32 a, F32 b, F32 c, F32 d, F32 e) override;
        void MODEL_R_D_cmdHandler(U32 opCode, U32 cmdSeq, F32 a, F32 b, F32 c, F32 d, F32 e) override;

        void MODEL_SIZE_cmdHandler(U32 opCode, U32 cmdSeq, U32 width, U32 height) override;

        void CAPTURE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                const Fw::CmdStringArg &location, Heli::CamSelect eye,
                                Heli::ImageEncoding encoding) override;

        void sched_handler(NATIVE_INT_TYPE portNum, NATIVE_UINT_TYPE context) override;

    PRIVATE:
        Calibration m_calib;
        std::vector<std::unique_ptr<VisStage>> m_stages;

        bool is_capturing;
        struct {
            Fw::Time request_time;

            Fw::String location;
            Heli::CamSelect eye;
            Heli::ImageEncoding encoding;

            FwOpcodeType opCode;
            U32 cmdSeq;
        } m_capture;
    };
}

#endif //STEREO_HELI_VIS_HPP
