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
        void RECTIFY_cmdHandler(U32 opCode, U32 cmdSeq, const Fw::CmdStringArg &calibration_file) override;
        void STEREO_cmdHandler(U32 opCode, U32 cmdSeq,
                               Vis_StereoAlgorithm algorithm) override;
        void COLORMAP_cmdHandler(U32 opCode, U32 cmdSeq, Vis_ColorMap colormap, CamSelect select) override;

        std::vector<std::unique_ptr<VisStage>> m_stages;
    };
}

#endif //STEREO_HELI_VIS_HPP
