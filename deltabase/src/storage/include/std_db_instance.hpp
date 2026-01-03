//
// Created by poproshaikin on 28.11.25.
//

#ifndef DELTABASE_STD_DB_INSTANCE_HPP
#define DELTABASE_STD_DB_INSTANCE_HPP

#include "db_instance.hpp"
#include "io_manager.hpp"
#include "../../types/include/config.hpp"

namespace storage
{
    class StdDbInstance final : public IDbInstance
    {
        types::Config cfg_;
        std::unique_ptr<IIOManager> io_manager_;

        void
        init();

    public:
        explicit
        StdDbInstance(const types::Config& cfg);

        ~StdDbInstance() override;

        bool
        needs_stream(types::IPlanNode& plan_node) override;

        types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) override;

        void
        insert_row(
            const std::string& table_name,
            const std::string& schema_name,
            std::vector<types::DataToken> row
        ) override;

        types::MetaTable
        get_table(const std::string& table_name, const std::string& schema_name) override;

        types::MetaTable
        get_table(const types::TableIdentifier& identifier) override;

        const types::Config&
        get_config() const override;

        types::MetaSchema
        get_schema(const std::string& name) override;

        bool
        exists_table(const std::string& table_name, const std::string& schema_name) override;

        bool
        exists_table(const types::TableIdentifier& identifier) override;

        bool
        exists_db(const std::string& name) override;

        void
        create_table(
            const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& columns
        ) override;

    };
}

#endif