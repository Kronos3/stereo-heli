//
// Created by tumbar on 3/31/22.
//

#ifndef HELI_FRAMEPIPEIMPL_H
#define HELI_FRAMEPIPEIMPL_H

#include <Heli/FramePipe/FramePipeComponentAc.hpp>
#include <Heli/FramePipe/FppConstantsAc.hpp>
#include <Heli/Cam/CamFrame.hpp>

#include <Os/Mutex.hpp>

namespace Heli
{
    class FramePipe : public FramePipeComponentBase
    {
    public:
        explicit FramePipe(const char* compName);

        void init(NATIVE_INT_TYPE instance);

    PRIVATE:
        void camFrame_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;
        void frameIn_handler(NATIVE_INT_TYPE portNum, U32 frameId) override;

        void CLEAR_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void PUSH_cmdHandler(U32 opCode, U32 cmdSeq,
                             FramePipe_Component cmp,
                             FramePipe_ComponentType cmpType) override;
        void CHECK_cmdHandler(U32 opCode, U32 cmdSeq) override;
        void SHOW_cmdHandler(U32 opCode, U32 cmdSeq) override;

        void send_to_idx(U32 idx, U32 frameId);


        struct Fifo
        {
            Fifo();

            bool empty() const;
            bool full() const;
            void push(U32 frameId);
            U32 pop();

            void clear();

            U32 m_buffer[FramePipe_PIPELINE_QUEUE_N];
            U32 n;
        };

        Os::Mutex m_mutex;

        struct Stage
        {
            I32 inStage = -1;
            Fifo queue; // only for wait types
            FramePipe_Component component;
            FramePipe_ComponentType type;
        } m_pipeline[FramePipe_PIPELINE_N];

        U32 m_pipeline_n;
        bool m_valid;
    };
}

#endif //HELI_FRAMEPIPEIMPL_H
