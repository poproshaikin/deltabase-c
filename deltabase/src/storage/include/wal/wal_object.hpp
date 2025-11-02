#pragma once 

#include "../shared.hpp"
#include <concepts>
#include <cstdint>
#include <variant>
#include <string>
#include <filesystem>

namespace storage {
    enum class WalRecordType : uint64_t {
        INSERT = 1,
        UPDATE_BY_FILTER,
        DELETE_BY_FILTER,
        CREATE_TABLE,
        CREATE_SCHEMA,
        DROP_TABLE,
        DROP_SCHEMA
    };

    namespace detail {
        template <typename T>
        concept Wal_c = requires(const T x) 
        {
            T::type;
            x.lsn;
            x.serialize();
            x.estimate_size();

            requires std::same_as<decltype(T::type), const WalRecordType>;
            requires std::same_as<decltype(x.lsn), uint64_t>;
            requires std::same_as<decltype(x.serialize()), bytes_v>;
            requires std::same_as<decltype(x.estimate_size()), uint64_t>;
        };

        template <Wal_c... Ts> 
        using WalRecordVariant = std::variant<Ts...>;
    };

    struct InsertRecord {
        static constexpr WalRecordType type = WalRecordType::INSERT;

        uint64_t lsn;
        std::string table_id;
        bytes_v serialized_row;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct CreateSchemaRecord {
        static constexpr WalRecordType type = WalRecordType::CREATE_SCHEMA; 

        uint64_t lsn;
        std::string schema_id;
        bytes_v serialized_schema;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct DropSchemaRecord {
        static constexpr WalRecordType type = WalRecordType::DROP_SCHEMA;

        uint64_t lsn;
        std::string schema_id;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct CreateTableRecord {
        static constexpr WalRecordType type = WalRecordType::CREATE_TABLE;

        uint64_t lsn;
        std::string table_id;
        bytes_v serialized_table;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    using WalRecord = detail::WalRecordVariant<
        InsertRecord, 
        CreateSchemaRecord,   
        DropSchemaRecord,
        CreateTableRecord
    >;

    struct WalCheckpoint {
        uint64_t lsn;
    };

    struct WalLogfile {
        static constexpr unsigned long max_logfile_size = 4 * 1024 * 1024; 

        std::filesystem::path path;
        std::vector<WalRecord> records;

        WalLogfile(std::filesystem::path path);
        WalLogfile(WalLogfile& other) = delete;

        uint64_t first_lsn() const;
        uint64_t last_lsn() const;
        uint64_t size() const;

        static bool
        try_deserialize(bytes_v content, WalLogfile& out);
        bytes_v
        serialize();

        // restrict copying
        WalLogfile(const WalLogfile&) = delete;
        WalLogfile& operator=(const WalLogfile&) = delete;
        
        // allow moving
        WalLogfile(WalLogfile&&) = default;
        WalLogfile& operator=(WalLogfile&&) = default;

    private:
        friend class FileManager;
        WalLogfile() = default;
    };
}