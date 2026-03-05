//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_MANAGER_HPP
#define DELTABASE_WAL_MANAGER_HPP
#include "config.hpp"
#include "wal_io_manager.hpp"

namespace wal
{
    class WalManager
    {
        std::unique_ptr<IWalIOManager> wal_io_manager;

    public:
        explicit
        WalManager(const types::Config& cfg);


    };
}

#endif // DELTABASE_WAL_MANAGER_HPP
