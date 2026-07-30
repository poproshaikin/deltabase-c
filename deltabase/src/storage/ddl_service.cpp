//
// Created by poproshaikin on 6/11/26.
//

#include "ddl_service.hpp"

#include "wal_manager.hpp"

#include "../misc/include/exceptions.hpp"
#include "../misc/include/utils.hpp"
#include "../misc/include/convert.hpp"
#include "index_bplus_tree.hpp"
#include "BP_index_pager.hpp"

#include <assert.h>

namespace storage
{
    using namespace types;

    DDLService::DDLService(
        Config& cfg,
        BufferPool& buffer_pool,
        CatalogCache& catalog,
        wal::IWALManager& wal_manager,
        IIOManager& io_manager
    ) : cfg_(cfg),
        buffer_pool_(buffer_pool),
        catalog_(catalog),
        wal_manager_(wal_manager),
        io_manager_(io_manager)
    {
    }

    MetaSchema*
    DDLService::create_schema(const std::string& schema_name, txn::Transaction& txn)
    {
        MetaSchema ms;
        ms.id = UUID::make();
        ms.name = schema_name;
        ms.db_name = cfg_.db_name.value();

        CreateSchemaRecord record(ms);
        txn.append_log(record);

        return catalog_.save_schema(ms, txn.get_id());
    }

    bool
    DDLService::exists_schema(const std::string& schema_name)
    {
        return catalog_.exists_schema(schema_name);
    }

    MetaSchema*
    DDLService::get_schema(const std::string& name)
    {
        return catalog_.get_schema(name);
    }

    MetaColumn
    DDLService::resolve_column(const ColumnDefinition& column_def, const MetaTable& mt)
    {
        MetaColumn column = misc::convert(column_def);
        column.id = UUID::make();
        column.table_id = mt.id;

        bool is_pk = false;
        bool is_ai = false;
        std::optional<ForeignKeyConstraint> fk;

        for (const auto& c : column_def.constraints)
            if (std::holds_alternative<PrimaryKeyConstraint>(c))
                is_pk = true;
            else if (std::holds_alternative<AutoIncrementConstraint>(c))
                is_ai = true;
            else if (std::holds_alternative<ForeignKeyConstraint>(c))
                fk.emplace(std::get<ForeignKeyConstraint>(c));

        bool is_fk = fk.has_value();

        if (is_pk)
        {
            column.constraints.emplace_back(MetaNotNullConstraint());
            column.constraints.emplace_back(MetaPrimaryKeyConstraint{});
        }

        if (is_ai)
        {
            auto constraint = MetaAutoIncrementConstraint{
                .column_id = column.id, .table_id = mt.id, .sequence_id = UUID::null()};
            column.constraints.push_back(constraint);
        }

        if (is_fk)
        {
            auto referenced_schema = catalog_.get_schema(
                fk->referenced_table.schema_name.value_or(
                    SqlToken(cfg_.default_schema)).value);

            auto referenced_table = catalog_.get_table(
                fk->referenced_table.table_name,
                referenced_schema->id);

            auto referenced_column = referenced_table->get_column(fk->referenced_column.value);

            auto meta_constraint = MetaForeignKeyConstraint{
                .referenced_table_id = referenced_table->id,
                .referenced_column_id = referenced_column.id,
                .action = fk->action};

            column.constraints.push_back(meta_constraint);
        }

        return column;
    }

    MetaTable*
    DDLService::create_table(
        const std::string& table_name,
        const std::string& schema_name,
        const std::vector<ColumnDefinition>& columns,
        txn::Transaction& txn)
    {
        auto* schema = catalog_.get_schema(schema_name);

        MetaTable mt;
        mt.id = UUID::make();
        mt.name = table_name;
        mt.schema_id = schema->id;
        mt.last_rid = 0;
        mt.columns.reserve(columns.size());

        for (const auto& col_def : columns)
            mt.columns.push_back(resolve_column(col_def, mt));

        CreateTableRecord record(mt);
        txn.append_log(record);

        auto* saved = catalog_.save_table(std::move(mt), txn.get_id());

        for (auto& col : saved->columns)
        {
            if (col.has_constraint<MetaAutoIncrementConstraint>())
            {
                auto seq_id = create_sequence(
                    mt.name + "_" + col.name + "_seq",
                    schema->name,
                    txn);
                col.get_constraint<MetaAutoIncrementConstraint>()->sequence_id = seq_id;
            }
            else if (col.has_constraint<MetaPrimaryKeyConstraint>())
            {
                create_index(
                    table_name + "_" + col.name + "_pkey",
                    table_name,
                    col.name,
                    schema_name,
                    true,
                    txn);
            }
        }

        return saved;
    }

    bool
    DDLService::exists_table(const std::string& table_name, const std::string& schema_name)
    {
        return get_table(table_name, schema_name) != nullptr;
    }

    bool
    DDLService::exists_table(const TableIdentifier& identifier)
    {
        std::string schema_name = identifier.require_schema(cfg_);
        return exists_table(identifier.table_name.value, schema_name);
    }

    MetaTable*
    DDLService::get_table(const std::string& table_name, const std::string& schema_name)
    {
        auto* ms = get_schema(schema_name);
        if (!ms)
            return nullptr;
        return catalog_.get_table(table_name, ms->id);
    }

    MetaTable*
    DDLService::get_table(const TableIdentifier& identifier)
    {
        return get_table(
            identifier.table_name.value,
            identifier.schema_name.has_value()
                ? identifier.schema_name.value().value
                : cfg_.default_schema
        );
    }

    void
    DDLService::drop_table(const std::string& table_name,
                           const std::string& schema_name,
                           txn::Transaction& txn)
    {
        auto* table = get_table(table_name, schema_name);
        if (!table)
            throw EngineException("Table " + table_name + " does not exist",
                                  EngineException::Code::TABLE_NOT_EXISTS);

        const auto table_unchanged = *table;

        io_manager_.delete_mt(table_unchanged);
        catalog_.delete_table(table_unchanged.id, txn.get_id());

        DeleteTableRecord record(table_unchanged);
        txn.append_log(record);
    }

    DataRow
    DDLService::extend_row(const DataRow& old_row, const MetaColumn& new_column)
    {
        auto old_values = old_row.tokens;
        if (new_column.has_constraint<MetaNotNullConstraint>())
        {
            auto* default_constraint = new_column.get_constraint<MetaDefaultConstraint>();
            if (!default_constraint)
                throw std::logic_error(
                    "Semantic analyzer invariant violated: NOT NULL column without default");

            old_values.push_back(default_constraint->value);
        }
        else
        {
            old_values.push_back(DataToken({}, DataType::_NULL));
        }

        DataRow new_row = old_row;
        new_row.tokens = std::move(old_values);
        return new_row;
    }

    void
    DDLService::add_column(
        const std::string& table_name,
        const std::string& schema_name,
        const ColumnDefinition& column,
        txn::Transaction& txn)
    {
        auto* mt = get_table(table_name, schema_name);
        const auto unchanged_mt = *mt;
        auto data = buffer_pool_.get_table_data(mt->id);
        std::unordered_set<DataPageId> linked_pages;

        MetaColumn new_column = misc::convert(column);

        mt->columns.push_back(new_column);

        if (mt->live_rows == 0)
        {
            UpdateTableRecord update_table_record(unchanged_mt, *mt);
            txn.append_log(update_table_record);
            return;
        }

        for (auto* reading_page : data)
        {
            for (auto& row : reading_page->rows)
            {
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                DataRow new_row = extend_row(row, new_column);
                auto size = io_manager_.estimate_size(new_row);

                DataPage* destination =
                    reading_page->size + size <= DataPage::MAX_SIZE
                        ? reading_page
                        : buffer_pool_.prepare_dp(size, *mt, txn.get_id());

                const DataRow old_row = row;
                row.flags |= DataRowFlags::OBSOLETE;
                UpdateRecord update_record(mt->id, reading_page->id, old_row, row);
                txn.append_log(update_record);
                reading_page->last_lsn = txn.get_last_lsn();
                buffer_pool_.dirty_dp(reading_page->id, txn.get_id());

                if (destination != reading_page && linked_pages.insert(destination->id).second)
                {
                    destination->next = reading_page->next;
                    reading_page->next = destination->id;
                    buffer_pool_.dirty_dp(reading_page->id, txn.get_id());
                }

                InsertRecord insert_record(mt->id, destination->id, new_row);
                txn.append_log(insert_record);
                buffer_pool_.append_row(destination, *mt, new_row, txn.get_last_lsn(), txn.get_id());
            }
        }

        UpdateTableRecord update_table_record(unchanged_mt, *mt);
        txn.append_log(update_table_record);
    }

    UUID
    DDLService::create_sequence(
        const std::string& sequence_name,
        const std::string& schema_name,
        txn::Transaction& txn)
    {
        auto* ms = catalog_.get_schema(schema_name);

        MetaSequence sequence{};
        sequence.id = UUID::make();
        sequence.name = sequence_name;
        sequence.schema_id = ms->id;
        sequence.schema_name = ms->name;
        sequence.current_value = 0;

        catalog_.put(sequence, txn.get_id());
        CreateSequenceRecord record(sequence);
        txn.append_log(record);

        return sequence.id;
    }

    void
    DDLService::create_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& column_name,
        const std::string& schema_name,
        bool is_unique,
        txn::Transaction& txn
    )
    {
        const auto* schema = catalog_.get_schema(schema_name);
        auto* table = catalog_.get_table(table_name, schema->id);
        const auto& column = table->get_column(column_name);

        MetaIndex mi;
        mi.id = UUID::make();
        mi.name = index_name;
        mi.column_id = column.id;
        mi.key_type = column.type;
        mi.is_unique = is_unique;
        mi.table_id = table->id;

        CreateIndexRecord record(mi);
        txn.append_log(record);

        buffer_pool_.create_table_index(schema_name, *table, mi, txn.get_last_lsn());

        const auto col_idx = table->get_column_idx(column.id);
        if (col_idx < 0)
            throw std::runtime_error("Index column not found in table schema");

        // Pre-validate unique constraint on existing data before creating index file
        if (is_unique)
        {
            auto pages = buffer_pool_.get_table_data(table->id);
            std::unordered_set<std::string> seen_values;

            for (const auto& page : pages)
            {
                for (const auto& row : page->rows)
                {
                    if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                        continue;

                    const auto& key = row.tokens[static_cast<size_t>(col_idx)];

                    // NULL values are not indexed and don't violate uniqueness
                    if (key.type == DataType::_NULL)
                        continue;

                    std::string key_str(key.bytes.begin(), key.bytes.end());

                    if (seen_values.contains(key_str))
                    {
                        throw EngineException(
                            "Cannot create unique index '" + index_name + "' on column '" +
                            column_name + "': table '" + table_name + "' contains duplicate values",
                            EngineException::Code::UNIQUE_VIOLATION
                        );
                    }

                    seen_values.insert(key_str);
                }
            }
        }

        BPIndexPager pager(buffer_pool_, table->id, mi.id);
        IndexBPlusTree tree(pager);

        auto pages = buffer_pool_.get_table_data(table->id);

        for (const auto& page : pages)
        {
            for (const auto& row : page->rows)
            {
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                const auto& key = row.tokens[static_cast<size_t>(col_idx)];

                // NULL values are not indexed
                if (key.type == DataType::_NULL)
                    continue;

                const RowPtr row_ptr{page->id, row.id};

                tree.insert(key, row_ptr);
            }
        }

        buffer_pool_.set_if_lsn(mi.id, txn.get_last_lsn());

        table->indexes.push_back(std::move(mi));
    }

    bool
    DDLService::exists_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& schema_name
    )
    {
        return get_index(index_name, table_name, schema_name) != nullptr;
    }

    bool
    DDLService::exists_index(
        const std::string& index_name,
        const TableIdentifier& table_identifier
    )
    {
        return get_index(index_name, table_identifier) != nullptr;
    }

    MetaIndex*
    DDLService::get_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& schema_name
    )
    {
        const auto* schema = catalog_.get_schema(schema_name);
        auto* table = catalog_.get_table(table_name, schema->id);

        for (auto& index : table->indexes)
            if (index.name == index_name)
                return &index;

        return nullptr;
    }

    MetaIndex*
    DDLService::get_index(
        const std::string& index_name,
        const TableIdentifier& identifier
    )
    {
        std::string schema_name = identifier.require_schema(cfg_);
        return get_index(index_name, identifier.table_name.value, schema_name);
    }

    void
    DDLService::drop_index(
        const std::string& index_name,
        const std::string& table_name,
        const std::string& schema_name,
        txn::Transaction& txn
    )
    {
        auto* table = get_table(table_name, schema_name);
        auto* index = get_index(index_name, table_name, schema_name);
        if (!index)
            throw std::runtime_error("DDLService::drop_index: index not found");

        const auto index_unchanged = *index;

        std::erase_if(table->indexes,
                      [&index](MetaIndex& index_entry)
                      {
                          return index_entry.id == index->id;
                      });

        DropIndexRecord record(index_unchanged);
        txn.append_log(record);
    }

    std::vector<MetaTable*>
    DDLService::get_all_tables() const
    {
        return catalog_.get_all_tables();
    }

    std::vector<MetaSchema*>
    DDLService::get_all_schemas() const
    {
        return catalog_.get_all_schemas();
    }

    void
    DDLService::throw_if_referenced(const TableId& table_id) const
    {
        const auto* mt = catalog_.get_table(table_id);

        for (const auto* other : catalog_.get_all_tables())
        {
            if (other->id == table_id)
                continue;

            for (const auto& col : other->columns)
                if (const auto* fk = col.get_constraint<MetaForeignKeyConstraint>())
                    if (fk->referenced_table_id == table_id)
                        throw EngineException(
                            "cannot drop table '" + mt->name + "': referenced by column '" +
                            col.name + "' in table '" + other->name + "'",
                            EngineException::Code::FOREIGN_KEY_VIOLATION);
    }
}