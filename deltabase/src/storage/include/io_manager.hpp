//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_IO_MANAGER_HPP
#define DELTABASE_IO_MANAGER_HPP
#include <vector>
#include "../../types/include/meta_table.hpp"
#include "../../types/include/meta_schema.hpp"

namespace storage
{
    class IIOManager
    {
    public:
        virtual ~IIOManager() = default;

        virtual std::vector<types::MetaTable>
        load_tables() = 0;

        virtual std::vector<types::MetaSchema>
        load_schemas() = 0;
    };
}

#endif //DELTABASE_IO_MANAGER_HPP