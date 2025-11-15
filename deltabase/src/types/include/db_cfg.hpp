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
        std::string default_schema = "common";
    };
}

#endif //DELTABASE_DB_CFG_HPP