//
// Created by poproshaikin on 5/3/26.
//

#ifndef DELTABASE_INFORMATION_SCHEMA_PROVIDER_HPP
#define DELTABASE_INFORMATION_SCHEMA_PROVIDER_HPP

#include "ast_tree.hpp"
#include "data_table.hpp"
#include "../../storage/include/db_instance.hpp"

namespace exq
{
    class InformationSchemaProvider
    {
        storage::IDbInstance& db_;

    public:
        InformationSchemaProvider(storage::IDbInstance& db);

        bool
        is_virtual(const types::TableIdentifier& table) const;

        bool
        is_virtual(const std::optional<std::string>& schema_name,
                   const std::string& table_name) const;

        types::MetaTable
        get_virtual_table(const types::TableIdentifier& table) const;

        types::MetaTable
        get_virtual_table(const std::string& schema_name, const std::string& table_name) const;

        types::DataTable
        get_virtual_data(const types::TableIdentifier& table) const;

        types::DataTable
        get_virtual_data(const std::string& schema_name, const std::string& table_name) const;
    };
}

#endif //DELTABASE_INFORMATION_SCHEMA_PROVIDER_HPP