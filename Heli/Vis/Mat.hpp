//
// Created by tumbar on 3/5/23.
//

#ifndef STEREO_HELI_MAT_HPP
#define STEREO_HELI_MAT_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/Serializable.hpp>

#include <opencv2/core/core.hpp>

namespace Heli
{
    class Mat : public Fw::Serializable
    {
    public:
        enum
        {
            SERIALIZED_SIZE =
                    sizeof(U32) + // row
                    sizeof(U32) + // col
                    sizeof(I32) + // type
                    sizeof(U32) + // step
                    sizeof(U64)   // data
        };

        explicit Mat(cv::Mat& mat);

        Fw::SerializeStatus serialize(Fw::SerializeBufferBase &buffer) const override;
        Fw::SerializeStatus deserialize(Fw::SerializeBufferBase &buffer) override;

        cv::Mat& get();
        const cv::Mat& get() const;

    private:
        cv::Mat m_mat;
    };
}

#endif //STEREO_HELI_MAT_HPP
