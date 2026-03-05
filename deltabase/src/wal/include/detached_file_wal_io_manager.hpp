//
// Created by poproshaikin on 05.03.26.
//

#ifndef DELTABASE_DETACHED_FILE_WAL_IO_MANAGER_HPP
#define DELTABASE_DETACHED_FILE_WAL_IO_MANAGER_HPP
#include "wal_io_manager.hpp"

namespace wal
{
    class DetachedFileWalIOManager : public IWalIOManager
    {
    public:
        void
        append_log(const types::WalRecord& record) override
        {
            throw std::runtime_error("WalIOManagerFactory::append_log(): not implemented");
        }

        void
        flush() override
        {
            throw std::runtime_error("WalIOManagerFactory::flush(): not implemented");
        }
    };
}

#endif // DELTABASE_DETACHED_FILE_WAL_IO_MANAGER_HPP
