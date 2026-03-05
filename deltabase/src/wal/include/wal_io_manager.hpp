//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_IO_MANAGER_HPP
#define DELTABASE_WAL_IO_MANAGER_HPP
#include "wal_log.hpp"

namespace wal
{
    class IWalIOManager
    {
    public:
        virtual ~IWalIOManager() = default;

        virtual void
        append_log(const types::WalRecord& record) = 0;

        virtual void
        flush() = 0;
    };
} // namespace wal

#endif