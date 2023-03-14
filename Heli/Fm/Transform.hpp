//
// Created by tumbar on 3/5/23.
//

#ifndef STEREO_HELI_TRANSFORM_HPP
#define STEREO_HELI_TRANSFORM_HPP

#include <Heli/Vis/Mat.hpp>

namespace Heli
{
    class Transform : public Fw::Serializable
    {
    public:
        enum
        {
            SERIALIZED_SIZE = sizeof(F32) * (9 + 3)
        };

        explicit Transform();
        explicit Transform(bool valid);
        explicit Transform(const cv::Mat& r, const cv::Mat& t);
        Transform(const Transform& tf);

        Fw::SerializeStatus serialize(Fw::SerializeBufferBase &buffer) const override;
        Fw::SerializeStatus deserialize(Fw::SerializeBufferBase &buffer) override;

        bool is_valid() const;

        cv::Mat r() { return m_r; }
        const cv::Mat& r() const  { return m_r; }

        cv::Mat t() { return m_t; }
        const cv::Mat& t() const { return m_t; }

        Transform inverse() const;

        /**
         * Perform Rx + t
         * @param x
         * @return Rx + t
         */
        cv::Mat operator*(const cv::Mat& x) const;
        Transform operator*(const Transform& tf);
        Transform operator=(const Transform& tf);

    private:
        bool m_valid;

        F32 m_r_raw[9];
        F32 m_t_raw[3];

        cv::Mat m_r;
        cv::Mat m_t;
    };
}

#endif //STEREO_HELI_TRANSFORM_HPP
