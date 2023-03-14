//
// Created by tumbar on 3/5/23.
//

#include "Transform.hpp"

#include <utility>

namespace Heli
{
    Fw::SerializeStatus Transform::serialize(Fw::SerializeBufferBase &buffer) const
    {
        F32 tf_raw[4][4];
        for (I32 i = 0; i < 4; i++)
        {
            for (I32 j = 0; j < 4; j++)
            {
                tf_raw[i][j] = m_tf(i)(j);
            }
        }

        NATIVE_UINT_TYPE size = sizeof(tf_raw);
        return buffer.serialize(reinterpret_cast<const U8*>(tf_raw), size, true);
    }

    Fw::SerializeStatus Transform::deserialize(Fw::SerializeBufferBase &buffer)
    {
        F32 tf_raw[4][4];
        NATIVE_UINT_TYPE size = sizeof(tf_raw);
        auto status = buffer.deserialize(reinterpret_cast<U8*>(tf_raw), size, true);
        if (status != Fw::SerializeStatus::FW_SERIALIZE_OK) return status;

        m_tf = (cv::Mat4f() <<
                tf_raw[0][0], tf_raw[0][0], tf_raw[0][0],
                tf_raw[1][0], tf_raw[1][1], tf_raw[1][2],
                tf_raw[2][0], tf_raw[2][1], tf_raw[2][2]
                );

        return status;
    }

    Transform::Transform()
    : m_tf(cv::Mat4f::eye(4, 4)),
      m_valid(true)
    {
    }

    Transform::Transform(const cv::Mat3f &r, const cv::Vec3f &t)
    : m_valid(true)
    {
        m_tf = (cv::Mat4f() <<
                r(0)(0), r(0)(1), r(0)(2), t(0),
                r(1)(0), r(1)(1), r(1)(2), t(1),
                r(2)(0), r(2)(1), r(2)(2), t(2),
                0, 0, 0, 1
        );
    }

    Transform::Transform(cv::Mat4f tf)
            : m_valid(true), m_tf(std::move(tf))
    {
    }

    Transform Transform::inverse() const
    {
        const cv::Mat4f& t = m_tf;
        return Transform((cv::Mat4f() <<
                t(0)(0), t(1)(0), t(2)(0), -t(0),
                t(0)(1), t(1)(1), t(2)(1), -t(1),
                t(0)(2), t(1)(2), t(2)(2), -t(2),
                0, 0, 0, 1
        ));
    }

    bool Transform::is_valid() const
    {
        return m_valid;
    }

    Transform::Transform(bool valid)
    : Transform()
    {
        m_valid = valid;
    }

    cv::Mat Transform::operator*(const cv::Vec3f &v) const
    {
        return m_tf * cv::Vec4f(v(0), v(1), v(2), 1);
    }

    cv::Mat Transform::operator*(const cv::Vec4f &v) const
    {
        return m_tf * v;
    }

    Transform Transform::operator*(const Transform &tf)
    {
        return Transform(m_tf * tf.tf());
    }

    Transform Transform::operator=(const Transform &tf)
    {
        m_tf = tf.m_tf;
        return *this;
    }

    cv::Mat Transform::R() const
    {
        const auto& t = *this;
        return (cv::Mat3f(3, 3)
                << t(0, 0), t(0, 1), t(0, 2),
                t(1, 0), t(1, 1), t(1, 2),
                t(2, 0), t(2, 1), t(2, 2));
    }

    F32 Transform::operator()(I32 i, I32 j) const
    {
        return m_tf.at<F32>(i, j);
    }

    cv::Vec3f Transform::t() const
    {
        const auto& t = *this;
        return {t(3, 0), t(3, 1), t(3, 2)};
    }

    Transform::Transform(const Transform &tf)
    : m_tf(tf.m_tf.clone()), m_valid(tf.m_valid)
    {
    }
}
