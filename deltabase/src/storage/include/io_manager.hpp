//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_IO_MANAGER_HPP
#define DELTABASE_IO_MANAGER_HPP
#include <vector>
#include "../../types/include/data_page.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/config.hpp"

namespace storage
{
    class IIOManager
    {
    public:
        virtual ~IIOManager() = default;

        virtual void
        init() = 0;

        virtual std::vector<types::MetaTable>
        load_tables_meta() = 0;

        virtual std::vector<types::MetaSchema>
        load_schemas_meta() = 0;

        virtual types::MetaSchema
        load_schema_meta(const std::string& schema_name) = 0;

        virtual bool
        exists_table(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::MetaTable
        load_table_meta(const std::string& table_name, const std::string& schema_name) = 0;

        virtual std::vector<types::DataPage>
        load_table_data(const std::string& table_name, const std::string& schema_name) = 0;

        virtual std::vector<std::pair<types::Uuid, std::vector<types::DataPage> > >
        load_tables_data() = 0;

        virtual uint64_t
        estimate_size(const types::DataRow& row) = 0;

        virtual constexpr uint64_t
        max_dp_size() = 0;

        virtual void
        write_page(const types::DataPage& page) = 0;

        virtual void
        write_mt(const types::MetaTable& table, const std::string& schema_name) = 0;

        virtual void
        write_cfg(const types::Config& cfg) = 0;

        virtual bool
        exists_db(const std::string& name) = 0;
    };
}

#endif //DELTABASE_IO_MANAGER_HPP