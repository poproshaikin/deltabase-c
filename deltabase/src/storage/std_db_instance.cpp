//
// Created by poproshaikin on 26.11.25.
//

#include "std_db_instance.hpp"

#include "io_manager_factory.hpp"
#include "logger.hpp"
#include "std_binary_serializer.hpp"
#include "utils.hpp"

#include "../misc/include/convert.hpp"
#include "../types/include/config.hpp"
#include "../wal/include/wal_manager_factory.hpp"

#include <unordered_set>

namespace storage
{
    using namespace types;
    using namespace misc;

    StdDbInstance::StdDbInstance(const Config& cfg) : cfg_(cfg)
    {
        if (!std::filesystem::exists(cfg.db_path))
            std::filesystem::create_directories(cfg.db_path);

        IOManagerFactory io_factory;
        io_manager_ = io_factory.make(cfg);
        wal::WalManagerFactory wal_factory;
        wal_manager_ = wal_factory.make(cfg);
        txn_manager_ = std::make_unique<txn::TransactionManager>(*wal_manager_);
        recovery_manager_ =
            std::make_unique<recovery::RecoveryManager>(cfg_, *wal_manager_, *io_manager_);

        init();
    }

    void
    StdDbInstance::init()
    {
        io_manager_->init();
        recovery_manager_->recover();
    }

    StdDbInstance::~StdDbInstance()
    {
        io_manager_->write_cfg(cfg_);
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
        auto mt = io_manager_->read_table_meta(table_name, schema_name);
        auto pages = io_manager_->read_table_data(table_name, schema_name);

        DataTable dt;
        dt.output_schema = convert(mt);

        uint64_t rows_count = 0;
        for (const auto& page : pages)
            rows_count += page.rows.size();

        dt.rows.reserve(rows_count);

        for (const auto& page : pages)
        {
            for (const auto& row : page.rows)
            {
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                dt.rows.push_back(row);
            }
        }

        return dt;
    }

    txn::Transaction
    StdDbInstance::make_txn()
    {
        return txn_manager_->make_transaction();
    }

    ssize_t
    StdDbInstance::has_available_page(const std::vector<DataPage>& vec, size_t size) const
    {
        if (vec.size() == 0)
            return -1;

        for (size_t i = 0; i < vec.size(); i++)
            if (vec[i].size + size <= DataPage::MAX_SIZE)
                return i;

        return -1;
    }

    void
    StdDbInstance::insert_row(
        const std::string& table_name,
        const std::string& schema_name,
        std::vector<DataToken> row,
        txn::Transaction& txn
    )
    {
        const auto old_table = io_manager_->read_table_meta(table_name, schema_name);
        auto updated_table = old_table;
        auto pages = io_manager_->read_table_data(table_name, schema_name);

        DataRow data_row;
        data_row.id = updated_table.last_rid++;
        data_row.tokens = std::move(row);

        size_t row_size = io_manager_->estimate_size(data_row);

        if (int idx = has_available_page(pages, row_size); idx == -1)
            pages.emplace_back(io_manager_->create_page(old_table));

        int idx = has_available_page(pages, row_size);
        if (idx == -1)
            throw std::runtime_error("StdDbInstance::insert_row: Failed to create new data page");

        pages[idx].rows.push_back(data_row);

        InsertRecord insert_record(0, 0, txn.get_id(), old_table.id, pages[idx].header.id, data_row);
        UpdateTableRecord update_table_record(0, 0, txn.get_id(), old_table, updated_table);
        txn.append_log(insert_record);
        txn.append_log(update_table_record);

        io_manager_->write_page(pages[idx]);
        io_manager_->write_mt(updated_table, schema_name);
    }

    void
    StdDbInstance::update_row(
        const std::string& table_name,
        const std::string& schema_name,
        RowUpdate update,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn
    )
    {
        const auto old_table = io_manager_->read_table_meta(table_name, schema_name);
        auto updated_table = old_table;
        std::vector<DataPage> pages = io_manager_->read_table_data(table_name, schema_name);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (auto& page : pages)
        {
            bool updated = false;

            for (auto& row : page.rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                DataRow new_row = row;
                new_row.id = ++updated_table.last_rid;

                row.flags |= DataRowFlags::OBSOLETE;

                for (const auto& assignment : update)
                {
                    ColumnId col_id = std::visit([](auto& a) { return a.first; }, assignment);
                    int64_t col_idx = old_table.get_column_idx(col_id);

                    MetaColumn cola = old_table.get_column(col_idx);

                    if (auto* lit = std::get_if<AssignLiteral>(&assignment))
                    {
                        new_row.tokens[col_idx] = lit->second;
                    }
                    else
                    {
                        auto* col = std::get_if<AssignColumn>(&assignment);
                        int src_idx = old_table.get_column_idx(col->second);
                        new_row.tokens[col_idx] = row.tokens[src_idx];
                    }
                }

                UpdateRecord record(0, 0, txn.get_id(), old_table.id, page.header.id, row, new_row);
                txn.append_log(record);

                page.rows.push_back(new_row);
                page.header.max_rid = std::max(page.header.max_rid, new_row.id);
                updated = true;
            }

            if (updated)
            {
                io_manager_->write_page(page);
                UpdateTableRecord record(0, 0, txn.get_id(), old_table, updated_table);
                txn.append_log(record);
            }
        }

        io_manager_->write_mt(updated_table, schema_name);
    }

    void
    StdDbInstance::delete_rows(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<DataRow>& rows,
        txn::Transaction& txn
    )
    {
        const auto table = io_manager_->read_table_meta(table_name, schema_name);
        std::vector<DataPage> pages = io_manager_->read_table_data(table_name, schema_name);

        std::unordered_set<RowId> ids;
        for (const auto& row : rows)
            ids.insert(row.id);

        for (auto& page : pages)
        {
            bool deleted = false;

            for (auto& row : page.rows)
            {
                if (!ids.contains(row.id))
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                deleted = true;

                row.flags |= DataRowFlags::OBSOLETE;

                DeleteRecord record(0, 0, txn.get_id(), table.id, page.header.id, row);
                txn.append_log(record);
            }

            if (deleted) 
                io_manager_->write_page(page);
        }

    }

    MetaTable
    StdDbInstance::get_table(const std::string& table_name, const std::string& schema_name)
    {
        return io_manager_->read_table_meta(table_name, schema_name);
    }

    MetaTable
    StdDbInstance::get_table(const TableIdentifier& identifier)
    {
        return get_table(
            identifier.table_name.value,
            identifier.schema_name.has_value() ? identifier.schema_name.value().value
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
        return io_manager_->read_schema_meta(name);
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
        const std::vector<ColumnDefinition>& columns
    )
    {
        auto schema = io_manager_->read_schema_meta(schema_name);

        MetaTable mt;
        mt.id = Uuid::make();
        mt.name = table_name;
        mt.schema_id = schema.id;
        mt.last_rid = 0;
        mt.columns.reserve(columns.size());
        for (const auto& col_def : columns)
        {
            MetaColumn column(col_def);
            column.id = Uuid::make();
            column.table_id = mt.id;
            mt.columns.emplace_back(column);
        }

        io_manager_->write_mt(mt, schema_name);
    }

    void
    StdDbInstance::create_schema(const std::string& schema_name)
    {
        MetaSchema ms;
        ms.id = Uuid::make();
        ms.name = schema_name;
        ms.db_name = cfg_.db_name.value();

        io_manager_->write_ms(ms);
    }

    bool
    StdDbInstance::exists_schema(const std::string& schema_name)
    {
        return io_manager_->exists_schema(schema_name);
    }
} // namespace storage