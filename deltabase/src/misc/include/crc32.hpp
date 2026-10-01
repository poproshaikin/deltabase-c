//
// Created by poproshaikin on 01.10.26.
//

#ifndef DELTABASE_CRC32_HPP
#define DELTABASE_CRC32_HPP
#include <cstddef>
#include <cstdint>

namespace misc
{
    using checksum_t = uint32_t;

    checksum_t
    crc32(const uint8_t* data, size_t size);
}

#endif //DELTABASE_CRC32_HPP