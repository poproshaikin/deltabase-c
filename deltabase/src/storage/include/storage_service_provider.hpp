//
// Created by poproshaikin on 6/11/26.
//

#ifndef DELTABASE_STORAGE_SERVICE_PROVIDER_HPP
#define DELTABASE_STORAGE_SERVICE_PROVIDER_HPP

#include "buffer_pool.hpp"
#include "catalog.hpp"
#include "io_manager.hpp"
#include "wal_manager.hpp"
#include "ddl_service.hpp"
#include "dml_service.hpp"
#include "dql_service.hpp"
#include "row_preprocessor.hpp"
#include "constraint_enforcer.hpp"
#include "../../types/include/config.hpp"
#include "../../recovery/include/recovery_manager.hpp"
#include "../../transactions/include/transaction_manager.hpp"
#include "../../transactions/include/transaction.hpp"

#include <memory>

namespace storage
{
    class StorageServiceProvider
    {
        types::Config cfg_;

        // infrastructure (must be declared before services)
        std::unique_ptr<IIOManager> io_manager_;
        std::unique_ptr<wal::IWALManager> wal_manager_;
        std::unique_ptr<BufferPool> buffer_pool_;
        std::unique_ptr<CatalogCache> catalog_;
        std::unique_ptr<recovery::RecoveryManager> recovery_manager_;
        std::unique_ptr<txn::TransactionManager> txn_manager_;

        // services (depend on infrastructure above)
        std::unique_ptr<DDLService> ddl_;
        std::unique_ptr<DMLService> dml_;
        std::unique_ptr<DqlService> dql_;
        std::unique_ptr<RowPreprocessor> row_preprocessor_;
        std::unique_ptr<ConstraintEnforcer> constraint_enforcer_;

    public:
        explicit
        StorageServiceProvider(const types::Config& cfg);

        ~StorageServiceProvider();

        const types::Config&
        config() const
        {
            return cfg_;
        }

        txn::Transaction
        make_txn();

        DDLService&
        ddl();
        DMLService&
        dml();
        DqlService&
        dql();
        RowPreprocessor&
        preprocessor();
        ConstraintEnforcer&
        enforcer();
    };
}

#endif //DELTABASE_STORAGE_SERVICE_PROVIDER_HPP