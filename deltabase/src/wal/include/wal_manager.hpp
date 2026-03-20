//
// Created by poproshaikin on 06.03.26.
//

#ifndef DELTABASE_I_WAL_MANAGER_HPP
#define DELTABASE_I_WAL_MANAGER_HPP

#include "../../types/include/wal_log.hpp"
#include <vector>

namespace wal
{
    class IWalManager
    {
    public:
        virtual ~IWalManager() = default;

        virtual void
        append_log(const types::WalRecord& record) = 0;

        virtual void
    append_log(const std::vector<types::WalRecord>& records) = 0;

        virtual types::WalRecord
        read_log(types::Lsn lsn) = 0;

        virtual void
        flush() = 0;

        virtual void
        sync() = 0;

        virtual std::vector<types::WalRecord>
        read_all_logs() = 0;

        virtual types::Lsn
        get_next_lsn() const = 0;
    };
} // namespace wal

#endif // DELTABASE_I_WAL_MANAGER_HPP
