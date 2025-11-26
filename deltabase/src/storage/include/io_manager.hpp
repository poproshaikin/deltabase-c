//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_IO_MANAGER_HPP
#define DELTABASE_IO_MANAGER_HPP
#include "std_data_buffers.hpp"
#include "../../types/include/data_table.hpp"

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
        load_tables_meta() = 0;

        virtual std::vector<types::MetaSchema>
        load_schemas_meta() = 0;

        virtual std::vector<std::pair<types::Uuid, std::vector<types::DataPage> > >
        load_tables_data() = 0;
    };
}

#endif //DELTABASE_IO_MANAGER_HPP