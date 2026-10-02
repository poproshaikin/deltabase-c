//
// Created by poproshaikin on 10/2/26.
//

#include "storage_registry.hpp"

namespace storage
{
    std::shared_ptr<StorageServiceProvider>
    StorageRegistry::acquire(const types::Config& cfg)
    {
        if (!cfg.db_name.has_value())
            throw std::runtime_error("StorageRegistry: db_name wasn't initialized");

        std::lock_guard lock(mutex_);
        const auto& key = make_key(cfg.db_path, cfg.db_name.value());

        auto it = providers_.find(key);
        if (it != providers_.end())
        {
            if (auto existing = it->second.lock())
                return existing;
        }

        auto provider = std::make_shared<StorageServiceProvider>(cfg);
        providers_[key] = provider;
        return provider;
    }

    std::string
    StorageRegistry::make_key(const std::filesystem::path& db_path, const std::string& db_name)
    {
        return db_path.lexically_normal().string() + "::" + db_name;
    }
}