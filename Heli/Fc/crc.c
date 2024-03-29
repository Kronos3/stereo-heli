/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include "crc.h"


uint16_t crc16_ccitt(uint16_t crc, unsigned char a)
{
    crc ^= (uint16_t)a << 8;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

uint16_t crc16_ccitt_update(uint16_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *pend = p + length;

    for (; p != pend; p++) {
        crc = crc16_ccitt(crc, *p);
    }
    return crc;
}

uint8_t crc8_dvb_s2(uint8_t crc, unsigned char a)
{
    crc ^= a;
    for (int ii = 0; ii < 8; ++ii) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

uint8_t crc8_dvb_s2_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *pend = p + length;

    for (; p != pend; p++) {
        crc = crc8_dvb_s2(crc, *p);
    }
    return crc;
}

uint8_t crc8_xor_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *pend = p + length;

    for (; p != pend; p++) {
        crc ^= *p;
    }
    return crc;
}

uint8_t crc8(uint8_t crc, uint8_t a)
{
    uint8_t crc_u = a;
    crc_u ^= crc;

    for (int i=0; i<8; i++) {
        crc_u = ( crc_u & 0x80 ) ? 0x7 ^ ( crc_u << 1 ) : ( crc_u << 1 );
    }

    return crc_u;
}

uint8_t crc8_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *pend = p + length;

    for (; p != pend; p++) {
        crc = crc8(crc, *p);
    }
    return crc;
}

uint8_t crc8_sum_update(uint8_t crc, const void *data, uint32_t length)
{
    const uint8_t *p = (const uint8_t *)data;
    const uint8_t *pend = p + length;

    for (; p != pend; p++) {
        crc += *p;
    }
    return crc;
}