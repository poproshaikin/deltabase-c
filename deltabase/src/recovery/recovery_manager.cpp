//
// Created by poproshaikin on 3/23/26.
//

#include "recovery_manager.hpp"

namespace recovery
{
    using namespace types;

    RecoveryManager::RecoveryManager(Config& cfg, wal::IWALManager& wal, storage::IIOManager& io)
        : cfg_(cfg), wal_(wal), io_(io)
    {
    }

    void
    RecoveryManager::recover()
    {
        auto wal = wal_.read_all_logs();

        for (const auto& record : wal)
        {
            apply(record);
        }
    }

    // ### APPLY ###

    void
    RecoveryManager::apply(const WALRecord& record)
    {
        LSN last_checkpoint = cfg_.last_checkpoint_lsn;

        std::visit(
            [&]<typename TRecord>(const TRecord& r)
            {
                using R = std::decay_t<TRecord>;

                // TODO:

                if (r.lsn <= last_checkpoint)
                    return;

                if constexpr (wal_log::has_page_id_v<R>)
                    apply_data(r);
                else
                    apply_meta(r);
            },
            record
        );
    }
    void
    RecoveryManager::apply_data(const WALDataRecord& record)
    {
        std::visit(
            [&](const auto& r)
            {
                auto page = io_.read_data_page(r.page_id);
                auto mt = io_.read_table_meta(r.table_id);
                if (!page)
                    page = std::make_unique<DataPage>(io_.create_page(mt));

                if (page->header.last_lsn < r.lsn)
                {
                    redo(r, *page);
                    page->header.last_lsn = r.lsn;
                    io_.write_page(*page);
                }
            },
            record
        );
    }
    void
    RecoveryManager::apply_meta(const WALRecord& record)
    {
        std::visit(
            [&]<typename TRecord>(const TRecord& r)
            {
                using R = std::decay_t<TRecord>;
                if constexpr (is_in_variant_v<R, WALMetaRecord>)
                {
                    if constexpr (std::is_same_v<R, CreateSchemaRecord>)
                    {
                        io_.write_ms(r.schema);
                    }
                    else if constexpr (std::is_same_v<R, CreateTableRecord>)
                    {
                        io_.write_mt(r.table);
                    }
                }
                else if constexpr (is_in_variant_v<R, WALTxnRecord>)
                {
                    if constexpr (std::is_same_v<R, BeginTxnRecord>)
                    {

                    }
                }
            },
            record
        );
    }

    // ### REDO ###

    void
    RecoveryManager::redo(const InsertRecord& record, DataPage& page)
    {
        page.rows.push_back(record.after);
    }
    void
    RecoveryManager::redo(const UpdateRecord& record, DataPage& page)
    {
        bool found = false;

        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(DeleteRecord): failed to find old row " +
                std::to_string(record.before.id)
            );

        page.rows.push_back(record.after);
    }
    void
    RecoveryManager::redo(const DeleteRecord& record, DataPage& page)
    {
        bool found = false;
        for (auto& row : page.rows)
        {
            if (row.id != record.before.id)
                continue;

            row.flags |= DataRowFlags::OBSOLETE;
            found = true;
            break;
        }

        if (!found)
            throw std::runtime_error(
                "RecoveryManager::redo(DeleteRecord): failed to find old row " +
                std::to_string(record.before.id)
            );
    }

} // namespace recovery