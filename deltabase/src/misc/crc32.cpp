//
// Created by poproshaikin on 01.10.26.
//

#include "include/crc32.hpp"

#include <array>

namespace misc
{
    namespace
    {
        constexpr std::array<uint32_t, 256>
        make_table()
        {
            std::array<uint32_t, 256> table{};
            for (uint32_t i = 0; i < 256; i++)
            {
                uint32_t c = i;
                for (int k = 0; k < 8; k++)
                    c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                table[i] = c;
            }
            return table;
        }

        constexpr auto table = make_table();
    }

    checksum_t
    crc32(const uint8_t* data, size_t size)
    {
        checksum_t crc = 0xFFFFFFFFu;
        for (size_t i = 0; i < size; i++)
            crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);

        return crc ^ 0xFFFFFFFFu;
    }
}