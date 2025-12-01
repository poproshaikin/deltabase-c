//
// Created by poproshaikin on 11.11.25.
//

#ifndef DELTABASE_STORAGE_HPP
#define DELTABASE_STORAGE_HPP
#include "../../types/include/data_row.hpp"
#include "../../types/include/data_table.hpp"
#include "../../types/include/config.hpp"
#include "../../types/include/query_plan.hpp"

namespace storage
{
    class IDbInstance
    {
    public:
        virtual ~IDbInstance() = default;

        virtual bool
        needs_stream(types::IPlanNode& plan_node) = 0;

        virtual types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) = 0;

        virtual void
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            std::vector<types::DataToken> row
        ) = 0;

        virtual types::MetaTable
        get_table(const std::string& table_name, const std::string& schema_name) = 0;

        virtual types::MetaTable
        get_table(types::TableIdentifier identifier) = 0;

        virtual const types::Config&
        get_config() const = 0;
    };
}

#endif //DELTABASE_STORAGE_HPP