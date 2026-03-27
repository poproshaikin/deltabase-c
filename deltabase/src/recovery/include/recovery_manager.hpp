//
// Created by poproshaikin on 3/23/26.
//

#ifndef DELTABASE_RECOVERY_MANAGER_HPP
#define DELTABASE_RECOVERY_MANAGER_HPP
#include "../../storage/include/io_manager.hpp"
#include "../../wal/include/wal_manager.hpp"

namespace recovery
{
    class RecoveryManager
    {
        types::Config& cfg_;
        wal::IWALManager& wal_;
        storage::IIOManager& io_;

        void
        apply(const types::WALRecord& record);
        void
        apply_data(const types::WALDataRecord& record);
        void
        apply_meta(const types::WALRecord& record);

        void
        redo(const types::InsertRecord& record, types::DataPage& page);
        void
        redo(const types::UpdateRecord& record, types::DataPage& page);
        void
        redo(const types::DeleteRecord& record, types::DataPage& page);

    public:
        explicit
        RecoveryManager(types::Config& cfg, wal::IWALManager& wal, storage::IIOManager& io);

        void
        recover();
    };
} // namespace recovery

#endif // DELTABASE_RECOVERY_MANAGER_HPP
