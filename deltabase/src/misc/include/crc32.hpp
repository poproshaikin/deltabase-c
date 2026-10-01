//
// Created by poproshaikin on 9/24/26.
//

#ifndef DELTABASE_CRC32_HPP
#define DELTABASE_CRC32_HPP

#include <cstddef>
#include <cstdint>

namespace misc
{
    using checksum_t = uint32_t;

    inline checksum_t
    crc32(const uint8_t* data, size_t len)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < len; i++)
        {
            crc ^= data[i];
            for (int j = 0; j < 8; j++)
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
        return ~crc;
    }
} // namespace misc

#endif // DELTABASE_CRC32_HPP