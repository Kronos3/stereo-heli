//
// Created by tumbar on 3/5/23.
//

#include "Transform.hpp"

namespace Heli
{
    Fw::SerializeStatus Transform::serialize(Fw::SerializeBufferBase &buffer) const
    {
        NATIVE_UINT_TYPE r_size = sizeof(m_r_raw);
        NATIVE_UINT_TYPE t_size = sizeof(m_t_raw);
        auto status = buffer.serialize(reinterpret_cast<const U8*>(m_r_raw), r_size, true);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(reinterpret_cast<const U8*>(m_t_raw), t_size, true);

        return status;
    }

    Fw::SerializeStatus Transform::deserialize(Fw::SerializeBufferBase &buffer)
    {
        NATIVE_UINT_TYPE r_size = sizeof(m_r_raw);
        NATIVE_UINT_TYPE t_size = sizeof(m_t_raw);
        auto status = buffer.deserialize(reinterpret_cast<U8*>(m_r_raw), r_size, true);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.deserialize(reinterpret_cast<U8*>(m_t_raw), t_size, true);

        m_r = cv::Mat(3, 3, CV_32F, m_r_raw);
        m_t = cv::Mat(3, 1, CV_32F, m_t_raw);

        return status;
    }

    Transform::Transform()
    : m_r_raw{1, 0, 0, 0, 1, 0, 0, 0, 1}, // identity
      m_t_raw{0, 0, 0},
      m_r(3, 3, CV_32F, m_r_raw),
      m_t(3, 1, CV_32F, m_t_raw),
      m_valid(true)
    {
    }

    Transform::Transform(const cv::Mat &r, const cv::Mat &t)
    : Transform()
    {
        r.copyTo(m_r);
        t.copyTo(m_t);
    }

    Transform Transform::inverse() const
    {
        return Transform(m_r.inv(), -1 * m_t);
    }

    Transform::Transform(const Transform &tf)
    : Transform(tf.r(), tf.t())
    {
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

    cv::Mat Transform::operator*(const cv::Mat &v) const
    {
        return r() * v + t();
    }

    Transform Transform::operator*(const Transform &tf)
    {
        return Transform(
                r() * tf.r(),
                t() + tf.t()
        );
    }

    Transform Transform::operator=(const Transform &tf)
    {
        tf.r().copyTo(m_r);
        tf.t().copyTo(m_t);

        return *this;
    }
}
