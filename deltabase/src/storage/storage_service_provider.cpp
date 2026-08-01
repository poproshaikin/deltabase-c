//
// Created by poproshaikin on 6/11/26.
//

#include "storage_service_provider.hpp"

#include "io_manager_factory.hpp"
#include "wal_manager_factory.hpp"
#include "../recovery/include/recovery_manager.hpp"
#include "../transactions/include/transaction_manager.hpp"

#include <filesystem>

namespace storage
{
    StorageServiceProvider::StorageServiceProvider(const types::Config& cfg) : cfg_(cfg)
    {
        if (!std::filesystem::exists(cfg_.db_path))
            std::filesystem::create_directories(cfg_.db_path);

        IOManagerFactory io_factory;
        io_manager_ = io_factory.make(cfg_);
        io_manager_->init();

        if (!cfg.db_name.has_value())
            return;

        io_manager_->init_wal();

        wal::WalManagerFactory wal_factory;
        wal_manager_ = wal_factory.make(cfg_);

        buffer_pool_ = std::make_unique<BufferPool>(*io_manager_);
        buffer_pool_->initialize();
        catalog_ = std::make_unique<CatalogCache>(*io_manager_);

        recovery_manager_ = std::make_unique<recovery::RecoveryManager>(
            cfg_,
            *wal_manager_,
            *io_manager_);

        txn_manager_ = std::make_unique<txn::TransactionManager>(
            *wal_manager_,
            *buffer_pool_,
            *catalog_,
            *recovery_manager_);

        if (cfg_.db_name.has_value())
            recovery_manager_->recover();

        catalog_->hydrate();

        ddl_ = std::make_unique<DDLService>(
            cfg_,
            *buffer_pool_,
            *catalog_,
            *wal_manager_,
            *io_manager_);

        dql_ = std::make_unique<DQLService>(*buffer_pool_);
        dml_ = std::make_unique<DMLService>(*ddl_, *buffer_pool_, *io_manager_);
        row_preprocessor_ = std::make_unique<RowPreprocessor>(*catalog_, *io_manager_);
        constraint_enforcer_ = std::make_unique<ConstraintEnforcer>(*dql_, *dml_, *catalog_);

        if (!ddl_->exists_schema(cfg_.default_schema))
        {
            auto txn = txn_manager_->make_transaction();
            txn.begin();
            ddl_->create_schema(cfg_.default_schema, txn);
            txn.commit();
        }
    }

    StorageServiceProvider::~StorageServiceProvider()
    {
        if (cfg_.db_name.has_value())
            io_manager_->write_cfg(cfg_);
    }

    txn::Transaction
    StorageServiceProvider::make_txn()
    {
        return txn_manager_->make_transaction();
    }

    DDLService&
    StorageServiceProvider::ddl()
    {
        return *ddl_;
    }

    DMLService&
    StorageServiceProvider::dml()
    {
        return *dml_;
    }

    DQLService&
    StorageServiceProvider::dql()
    {
        return *dql_;
    }

    RowPreprocessor&
    StorageServiceProvider::preprocessor()
    {
        return *row_preprocessor_;
    }

    ConstraintEnforcer&
    StorageServiceProvider::enforcer()
    {
        return *constraint_enforcer_;
    }
}