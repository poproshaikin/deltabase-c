//
// Created by poproshaikin on 6/11/26.
//

#include "dml_service.hpp"

namespace storage
{
    void
    DMLService::insert_row(const std::string& table_name, const std::string& schema_name, const std::optional<std::vector<std::string>>& cols, std::vector<types::DataToken> row, txn::Transaction& txn)
    {

        const auto* ms = catalog_->get_schema(schema_name);
        auto* mt = catalog_->get_table(table_name, ms->id);
        const auto mt_unchanged = *mt;

        auto effective_cols = cols;
        auto effective_row = row;
        fill_autoincrement_columns(*mt, effective_cols, effective_row, txn);

        // TODO: validate_fk(...);

        auto new_row = mt->make_row(effective_cols, effective_row);
        size_t row_size = io_manager_->estimate_size(new_row);

        auto* page = buffer_pool_->prepare_dp(row_size, *mt, txn.get_id());

        std::vector<IndexId> touched_indexes;
        if (mt->indexes.size() > 0)
            touched_indexes = insert_row_into_indexes(*mt, new_row, page->id);

        page->rows.push_back(new_row);

        InsertRecord insert_record(mt->id, page->id, new_row);
        UpdateTableRecord update_table_record(mt_unchanged, *mt);
        txn.append_log(insert_record);
        txn.append_log(update_table_record);
        const LSN page_lsn = txn.get_last_lsn();

        page->last_lsn = page_lsn;
        for (const auto& index_id : touched_indexes)
            buffer_pool_->set_if_lsn(index_id, page_lsn);

        buffer_pool_->dirty_dp(page->id, txn.get_id());
    }
}