//
// Created by poproshaikin on 9/24/26.
//

#ifndef DELTABASE_CONTROL_FILE_HPP
#define DELTABASE_CONTROL_FILE_HPP
#include "data_page.hpp"

namespace types
{
    struct ControlFile
    {
        LSN last_checkpoint_lsn;
        uint32_t crc32;
    };
}

#endif //DELTABASE_CONTROL_FILE_HPP
