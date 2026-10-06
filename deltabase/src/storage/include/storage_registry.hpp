//
// Created by poproshaikin on 10/2/26.
//

#ifndef DELTABASE_STORAGE_REGISTRY_HPP
#define DELTABASE_STORAGE_REGISTRY_HPP
#include "storage_service_provider.hpp"

#include <memory>

namespace storage
{
    class StorageRegistry
    {
    public:
        std::shared_ptr<StorageServiceProvider>
        acquire(const types::Config& cfg);

    private:
        using ProvidersMap = std::unordered_map<std::string, std::weak_ptr<StorageServiceProvider>>;

        std::mutex mutex_;
        ProvidersMap providers_;

        static std::string
        make_key(const std::filesystem::path& db_path, const std::string& db_name);
    };
}

#endif //DELTABASE_STORAGE_REGISTRY_HPP
