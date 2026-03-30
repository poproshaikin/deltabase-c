//
// Created by poproshaikin on 3/23/26.
//

#ifndef DELTABASE_RECOVERY_MANAGER_HPP
#define DELTABASE_RECOVERY_MANAGER_HPP
#include "../../storage/include/io_manager.hpp"
#include "../../wal/include/wal_manager.hpp"
#include "../../transactions/include/transaction.hpp"

namespace recovery
{
    class RecoveryManager
    {
        types::Config& cfg_;
        wal::IWALManager& wal_;
        storage::IIOManager& io_;

        void
        redo(
            const types::WALRecord& record,
            const std::unordered_map<txn::TxnId, types::LSN>& commit_lsns
        );
        void
        redo_data(const types::WALDataRecord& record);
        void
        redo_meta(const types::WALRecord& record);

        void
        redo(const types::InsertRecord& record, types::DataPage& page);
        void
        redo(const types::UpdateRecord& record, types::DataPage& page);
        void
        redo(const types::DeleteRecord& record, types::DataPage& page);
        void
        redo(const types::CLRInsertRecord& record, types::DataPage& page);
        void
        redo(const types::CLRUpdateRecord& record, types::DataPage& page);
        void
        redo(const types::CLRDeleteRecord& record, types::DataPage& page);

        void
        redo(const types::CreateSchemaRecord& record);
        void
        redo(const types::UpdateSchemaRecord& record);
        void
        redo(const types::DeleteSchemaRecord& record);
        void
        redo(const types::CLRCreateSchemaRecord& record);
        void
        redo(const types::CLRUpdateSchemaRecord& record);
        void
        redo(const types::CLRDeleteSchemaRecord& record);

        void
        redo(const types::CreateTableRecord& record);
        void
        redo(const types::UpdateTableRecord& record);
        void
        redo(const types::DeleteTableRecord& record);
        void
        redo(const types::CLRCreateTableRecord& record);
        void
        redo(const types::CLRUpdateTableRecord& record);
        void
        redo(const types::CLRDeleteTableRecord& record);

        void
        redo(const types::CreateIndexRecord& record);
        void
        redo(const types::CLRCreateIndexRecord& record);

        void
        redo(const types::BeginTxnRecord& record);
        void
        redo(const types::CommitTxnRecord& record);
        void
        redo(const types::RollbackTxnRecord& record);

        types::WALRecord
        make_clr(const types::InsertRecord& record) const;
        types::WALRecord
        make_clr(const types::UpdateRecord& record) const;
        types::WALRecord
        make_clr(const types::DeleteRecord& record) const;
        types::WALRecord
        make_clr(const types::CreateSchemaRecord& record) const;
        types::WALRecord
        make_clr(const types::UpdateSchemaRecord& record) const;
        types::WALRecord
        make_clr(const types::DeleteSchemaRecord& record) const;
        types::WALRecord
        make_clr(const types::CreateTableRecord& record) const;
        types::WALRecord
        make_clr(const types::UpdateTableRecord& record) const;
        types::WALRecord
        make_clr(const types::DeleteTableRecord& record) const;
        types::WALRecord
        make_clr(const types::CreateIndexRecord& record) const;
        types::WALRecord
        make_clr(const types::RollbackTxnRecord& record) const;

        std::unordered_map<txn::TxnId, types::LSN>
        get_commit_lsns(const std::vector<types::WALRecord>& wal) const;
        std::unordered_map<txn::TxnId, types::LSN>
        get_rollback_lsns(const std::vector<types::WALRecord>& wal) const;
        std::unordered_map<txn::TxnId, types::LSN>
        get_last_lsns(const std::vector<types::WALRecord>& wal) const;
        std::unordered_map<txn::TxnId, types::LSN>
        get_active_txns(
            const std::unordered_map<txn::TxnId, types::LSN>& last,
            const std::unordered_map<txn::TxnId, types::LSN>& commit,
            const std::unordered_map<txn::TxnId, types::LSN>& rollback);

        void
        undo(const std::unordered_map<txn::TxnId, types::LSN>& active_lsns);

        void
        undo_record(const types::InsertRecord& record, types::DataPage& page);
        void
        undo_record(const types::UpdateRecord& record, types::DataPage& page);
        void
        undo_record(const types::DeleteRecord& record, types::DataPage& page);
        void
        undo_record(const types::CreateSchemaRecord& record);
        void
        undo_record(const types::UpdateSchemaRecord& record);
        void
        undo_record(const types::DeleteSchemaRecord& record);
        void
        undo_record(const types::CreateTableRecord& record);
        void
        undo_record(const types::UpdateTableRecord& record);
        void
        undo_record(const types::DeleteTableRecord& record);
        void
        undo_record(const types::CreateIndexRecord& record);
        void
        undo_record(const types::RollbackTxnRecord& record);

    public:
        explicit
        RecoveryManager(types::Config& cfg, wal::IWALManager& wal, storage::IIOManager& io);

        void
        recover();
    };
} // namespace recovery

#endif // DELTABASE_RECOVERY_MANAGER_HPP
