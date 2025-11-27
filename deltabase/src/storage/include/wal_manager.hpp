//
// Created by poproshaikin on 27.11.25.
//

#ifndef DELTABASE_WAL_MANAGER_HPP
#define DELTABASE_WAL_MANAGER_HPP
#include <vector>

#include "../../types/include/wal_log.hpp"

namespace storage
{
    class IWalManager
    {
    public:
        virtual ~IWalManager() = default;

        virtual void
        push_log(const types::WalRecord& record);
    };
}

#endif //DELTABASE_WAL_MANAGER_HPP