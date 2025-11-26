//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_DB_CFG_HPP
#define DELTABASE_DB_CFG_HPP
#include <string>

namespace types
{
    struct DbConfig
    {
        enum class IOSysType
        {
            File = 1,
        };

        std::string default_schema = "common";
        IOSysType io_system_type = IOSysType::File;
    };
}

#endif //DELTABASE_DB_CFG_HPP