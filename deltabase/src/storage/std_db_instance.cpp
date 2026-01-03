//
// Created by poproshaikin on 26.11.25.
//

#include "std_db_instance.hpp"

#include "file_io_manager.hpp"
#include "logger.hpp"
#include "path.hpp"
#include "std_binary_serializer.hpp"

#include "../misc/include/convert.hpp"
#include "../types/include/config.hpp"

#include <fstream>

namespace storage
{
    using namespace types;
    using namespace misc;

    void
    StdDbInstance::init()
    {
        io_manager_->init();
    }

    StdDbInstance::StdDbInstance(const Config& cfg) : cfg_(cfg)
    {
        Logger::info("Initializing database instance");

        if (!std::filesystem::exists(cfg.db_path))
            std::filesystem::create_directories(cfg.db_path);

        switch (cfg.io_type)
        {
        case Config::IoType::File:
            io_manager_ = std::make_unique<FileIOManager>(cfg.db_path, cfg.serializer_type);
            break;
        default:
            throw std::runtime_error(
                "StdDbInstance::StdDbInstance: unknown IO type " + std::to_string(
                    static_cast<int>(cfg.io_type)
                )
            );
        }

        init();
        Logger::info("Initialization of a database instance completed");
    }

    StdDbInstance::~StdDbInstance()
    {
        io_manager_->write_cfg(cfg_);
        Logger::info("Destruction of a database instance completed");
    }

    bool
    StdDbInstance::needs_stream(IPlanNode& plan_node)
    {
        // TODO
        return false;
    }

    DataTable
    StdDbInstance::seq_scan(const std::string& table_name, const std::string& schema_name)
    {
        auto mt = io_manager_->load_table_meta(table_name, schema_name);
        auto pages = io_manager_->load_table_data(table_name, schema_name);

        DataTable dt;
        dt.output_schema = misc::convert(mt);

        uint64_t rows_count = 0;
        for (const auto& page : pages)
            rows_count += page.rows.size();

        dt.rows.reserve(rows_count);

        for (const auto& page : pages)
            for (const auto& row : page.rows)
                dt.rows.push_back(row);

        return dt;
    }

    void
    StdDbInstance::insert_row(
        const std::string& table_name,
        const std::string& schema_name,
        std::vector<DataToken> row
    )
    {
        auto table = io_manager_->load_table_meta(table_name, schema_name);
        auto pages = io_manager_->load_table_data(table_name, schema_name);

        DataRow data_row;
        data_row.id = table.last_rid++;
        data_row.tokens = std::move(row);

        for (auto& page : pages)
        {
            if (page.size + io_manager_->estimate_size(data_row) > io_manager_->max_dp_size())
                continue;

            page.rows.push_back(data_row);

            io_manager_->write_page(page);
            break;
        }

        io_manager_->write_mt(table, schema_name);
    }

    MetaTable
    StdDbInstance::get_table(const std::string& table_name, const std::string& schema_name)
    {
        return io_manager_->load_table_meta(table_name, schema_name);
    }

    MetaTable
    StdDbInstance::get_table(const TableIdentifier& identifier)
    {
        return get_table(
            identifier.table_name.value,
            identifier.schema_name.has_value()
                ? identifier.schema_name.value().value
                : cfg_.default_schema
        );
    }

    const Config&
    StdDbInstance::get_config() const
    {
        return cfg_;
    }

    MetaSchema
    StdDbInstance::get_schema(const std::string& name)
    {
        return io_manager_->load_schema_meta(name);
    }

    bool
    StdDbInstance::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        return io_manager_->exists_table(table_name, schema_name);
    }

    bool
    StdDbInstance::exists_table(const TableIdentifier& identifier)
    {
        std::string schema_name = identifier.schema_name.has_value()
                                      ? identifier.schema_name.value().value
                                      : cfg_.default_schema;

        return exists_table(identifier.table_name.value, schema_name);
    }

    bool
    StdDbInstance::exists_db(const std::string& name)
    {
        return io_manager_->exists_db(name);
    }

    void
    StdDbInstance::create_table(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<ColumnDefinition>& columns)
    {
        auto schema = io_manager_->load_schema_meta(schema_name);

        MetaTable mt;
        mt.name = table_name;
        mt.schema_id = schema.id;
        mt.last_rid = 0;
        mt.columns.reserve(columns.size());
        for (const auto& column : columns)
            mt.columns.emplace_back(MetaColumn(column));

        io_manager_->write_mt(mt, schema.name);
    }
}