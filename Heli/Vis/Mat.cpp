//
// Created by tumbar on 3/5/23.
//

#include "Mat.hpp"

namespace Heli
{
    Mat::Mat(cv::Mat &mat)
    : m_mat(mat)
    {
    }

    Fw::SerializeStatus Mat::serialize(Fw::SerializeBufferBase &buffer) const
    {
        Fw::SerializeStatus status;
        status = buffer.serialize(static_cast<U32>(m_mat.rows));
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(static_cast<U32>(m_mat.cols));
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(static_cast<I32>(m_mat.type()));
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(static_cast<U32>(m_mat.step));
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(reinterpret_cast<U64>(m_mat.data));
        return status;
    }

    Fw::SerializeStatus Mat::deserialize(Fw::SerializeBufferBase &buffer)
    {
        U32 row, col, step;
        I32 type;
        U64 data;

        Fw::SerializeStatus status;
        status = buffer.deserialize(row);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(col);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(type);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(step);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        status = buffer.serialize(data);
        if (status != Fw::FW_SERIALIZE_OK) return status;

        m_mat = cv::Mat(row, col, type,
                        reinterpret_cast<void*>(data), step);
        return status;
    }

    cv::Mat &Mat::get()
    {
        return m_mat;
    }

    const cv::Mat &Mat::get() const
    {
        return m_mat;
    }
}
