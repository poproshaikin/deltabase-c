//
// Created by coproshaikin on 04.04.26.
//

#ifndef DELTABASE_DB_IO_LOCK_SERVICE_HPP
#define DELTABASE_DB_IO_LOCK_SERVICE_HPP

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace storage
{
    namespace fs = std::filesystem;

    class DatabaseIoLockService
    {
        mutable std::mutex registry_mutex_;
        std::unordered_map<std::string, std::shared_ptr<std::recursive_mutex>> locks_;

        static std::string
        make_key(const fs::path& db_path, const std::string& db_name)
        {
            return db_path.lexically_normal().string() + "::" + db_name;
        }

    public:
        using Mutex = std::recursive_mutex;
        using MutexPtr = std::shared_ptr<Mutex>;

        static std::shared_ptr<DatabaseIoLockService>
        shared()
        {
            static auto service = std::make_shared<DatabaseIoLockService>();
            return service;
        }

        MutexPtr
        mutex_for(const fs::path& db_path, const std::string& db_name)
        {
            const auto key = make_key(db_path, db_name);

            std::lock_guard<std::mutex> guard(registry_mutex_);
            auto& mutex = locks_[key];
            if (!mutex)
                mutex = std::make_shared<Mutex>();

            return mutex;
        }
    };
} // namespace storage

#endif // DELTABASE_DB_IO_LOCK_SERVICE_HPP