//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_DETACHED_DB_INSTANCE_HPP
#define DELTABASE_DETACHED_DB_INSTANCE_HPP
#include "db_instance.hpp"
#include "io_manager.hpp"

namespace storage
{
    class DetachedDbInstance final : public IDbInstance
    {
        types::Config config_;
        std::unique_ptr<IIOManager> io_manager_;

    public:
        explicit
        DetachedDbInstance(const types::Config& config);

        bool
        exists_db(const std::string& db_name) override;

        const types::Config&
        get_config() const override;

        // Not supported methods

        bool
        needs_stream(types::IPlanNode& plan_node) override;

        types::DataTable
        seq_scan(const std::string& table_name, const std::string& schema_name) override;

        void
        insert_row(const std::string& table_name,
            const std::string& schema_name,
            std::vector<types::DataToken> row) override;

        bool
        exists_table(const std::string& table_name, const std::string& schema_name) override;

        bool
        exists_table(const types::TableIdentifier& identifier) override;

        types::MetaTable
        get_table(const std::string& table_name, const std::string& schema_name) override;

        types::MetaTable
        get_table(const types::TableIdentifier& identifier) override;

        types::MetaSchema
        get_schema(const std::string& name) override;

        void
        create_table(const std::string& table_name,
            const std::string& schema_name,
            const std::vector<types::ColumnDefinition>& vector) override;

        void
        create_schema(const std::string& schema_name) override;

        bool
        exists_schema(const std::string& schema_name) override;

        void
        update_row(
            const std::string& table_name,
            const std::string& schema_name,
            types::RowUpdate update,
            const std::vector<types::DataRow>& rows
        ) override;
    };
}

#endif //DELTABASE_DETACHED_DB_INSTANCE_HPP