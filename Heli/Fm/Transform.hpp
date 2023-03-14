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
            SERIALIZED_SIZE = sizeof(F32) * (4 * 4)
        };

        explicit Transform();
        explicit Transform(bool valid);
        explicit Transform(const cv::Mat3f& r, const cv::Vec3f& t);
        explicit Transform(cv::Mat4f  tf);
        Transform(const Transform& tf);

        Fw::SerializeStatus serialize(Fw::SerializeBufferBase &buffer) const override;
        Fw::SerializeStatus deserialize(Fw::SerializeBufferBase &buffer) override;

        bool is_valid() const;

        cv::Mat R() const;
        cv::Vec3f t() const;
        const cv::Mat4f& tf() const { return m_tf; }

        F32 operator()(I32 i, I32 j) const;

        Transform inverse() const;

        cv::Mat operator*(const cv::Vec3f& x) const;
        cv::Mat operator*(const cv::Vec4f& x) const;
        Transform operator*(const Transform& tf);
        Transform operator=(const Transform& tf);

    private:
        bool m_valid;

        cv::Mat4f m_tf;
    };
}

#endif //STEREO_HELI_TRANSFORM_HPP
