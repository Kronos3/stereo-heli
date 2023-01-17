//
// Created by tumbar on 12/29/22.
//

#ifndef STEREO_HELI_MSPCIRCULARBUFFER_HPP
#define STEREO_HELI_MSPCIRCULARBUFFER_HPP

#include <Utils/Types/CircularBuffer.hpp>
#include <FcCfg.hpp>

namespace Msp
{
    class CircularBuffer : public Types::CircularBuffer
    {
        /**
         * The MSP protocol works in little-endian while
         * FPrime serialization works in big-endian.
         *
         * This class overrides peek to allow reads of U16 and U32
         * in little-endian byte order
         */
    public:
        /**
         * Initial circular buffer with the internal static memory
         * Size is Heli::FcCfg::RING_BUFFER_SIZE, see FcCfg.hpp
         */
        CircularBuffer();

        /**
         * Deserialize data into the given variable without moving the head index
         * \param value: value to fill
         * \param offset: offset from head to start peak. Default: 0
         * \return Fw::FW_SERIALIZE_OK on success or something else on error
         */
        Fw::SerializeStatus peek(char& value, NATIVE_UINT_TYPE offset = 0) const;
        /**
         * Deserialize data into the given variable without moving the head index
         * \param value: value to fill
         * \param offset: offset from head to start peak. Default: 0
         * \return Fw::FW_SERIALIZE_OK on success or something else on error
         */
        Fw::SerializeStatus peek(U8& value, NATIVE_UINT_TYPE offset = 0) const;

        /**
         * Deserialize data into the given variable without moving the head index
         * \param value: value to fill
         * \param offset: offset from head to start peak. Default: 0
         * \return Fw::FW_SERIALIZE_OK on success or something else on error
         */
        Fw::SerializeStatus peek(U16& value, NATIVE_UINT_TYPE offset = 0) const;

        /**
         * Deserialize data into the given variable without moving the head index
         * \param value: value to fill
         * \param offset: offset from head to start peak. Default: 0
         * \return Fw::FW_SERIALIZE_OK on success or something else on error
         */
        Fw::SerializeStatus peek(U32& value, NATIVE_UINT_TYPE offset = 0) const;

        /**
         * Deserialize data into the given buffer without moving the head variable.
         * \param buffer: buffer to fill with data of the peek
         * \param size: size in bytes to peek at
         * \param offset: offset from head to start peak. Default: 0
         * \return Fw::FW_SERIALIZE_OK on success or something else on error
         */
        Fw::SerializeStatus peek(U8* buffer, NATIVE_UINT_TYPE size, NATIVE_UINT_TYPE offset = 0) const;

    PRIVATE:
        U8 m_ring[Heli::FcCfg::RING_BUFFER_SIZE];
    };
}

#endif //STEREO_HELI_MSPCIRCULARBUFFER_HPP
