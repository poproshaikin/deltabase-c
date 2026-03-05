//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_FILEWALIOMANAGER_HPP
#define DELTABASE_FILEWALIOMANAGER_HPP
#include "config.hpp"
#include "wal_io_manager.hpp"
#include "wal_serializer.hpp"

namespace wal
{
    namespace fs = std::filesystem;

    class FileWalIOManager : public IWalIOManager
    {
        fs::path db_path_;
        std::string db_name_;

        std::vector<types::WalRecord> flushed_;
        std::vector<types::WalRecord> dirty_;

        std::unique_ptr<IWalSerializer> serializer_;

        void
        write_logs(const std::vector<types::WalRecord>& log) const;

    public:
        explicit FileWalIOManager(
            const fs::path& db_path,
            const std::string& db_name,
            types::Config::SerializerType serializer_type
        );

        void
        append_log(const types::WalRecord& record) override;

        void
        flush() override;
    };
} // namespace wal

#endif // DELTABASE_FILEWALIOMANAGER_HPP
