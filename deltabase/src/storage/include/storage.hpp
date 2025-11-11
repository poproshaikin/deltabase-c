//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_STORAGE_HPP
#define DELTABASE_STORAGE_HPP
#include "../../types/include/meta_table.hpp"

#include <memory>

namespace storage
{
    class IStorage
    {
        virtual ~IStorage() = default;

        virtual std::shared_ptr<types::MetaTable>
        get_table(std::string table_name, std::string schema_name) = 0;
    };
}

#endif //DELTABASE_STORAGE_HPP