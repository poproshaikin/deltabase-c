//
// Created by poproshaikin on 07.03.26.
//

#ifndef DELTABASE_TRANSACTION_HPP
#define DELTABASE_TRANSACTION_HPP
#include "../../types/include/UUID.hpp"
#include "../../types/include/wal_log.hpp"

#include <cstdint>

namespace txn
{
    using TxnId = types::UUID;

    class TransactionManager;

    enum class TransactionState
    {
        IDLE = 0,
        ACTIVE,
        COMMITTED,
        ABORTED
    };

    class Transaction
    {
        TxnId id_;
        TransactionManager* mgr_;
        TransactionState state_ = TransactionState::IDLE;
        types::LSN last_lsn_ = 0;

        Transaction(const TxnId& id, TransactionManager& mgr);

        friend class TransactionManager;

    public:
        TxnId
        get_id() const;

        types::LSN
        get_last_lsn() const;

        void
        begin();

        void
        append_log(const types::WALRecord& record);

        void
        commit();

        void
        rollback();

    private:
        types::LSN
        undo_one(const types::BeginTxnRecord& record);
        types::LSN
        undo_one(const types::InsertRecord& record);
        types::LSN
        undo_one(const types::UpdateRecord& record);
        types::LSN
        undo_one(const types::DeleteRecord& record);
        types::LSN
        undo_one(const types::CreateSchemaRecord& record);
        types::LSN
        undo_one(const types::UpdateSchemaRecord& record);
        types::LSN
        undo_one(const types::DeleteSchemaRecord& record);
        types::LSN
        undo_one(const types::CreateTableRecord& record);
        types::LSN
        undo_one(const types::UpdateTableRecord& record);
        types::LSN
        undo_one(const types::DeleteTableRecord& record);
        types::LSN
        undo_one(const types::CreateIndexRecord& record);
        types::LSN
        undo_one(const types::DropIndexRecord& record);
        types::LSN
        undo_one(const types::CreateSequenceRecord& record);
        types::LSN
        undo_one(const types::UpdateSequenceRecord& record);

        template <typename R>
        types::LSN
        undo_one(const R& record);
    };
} // namespace txn

#endif // DELTABASE_TRANSACTION_HPP