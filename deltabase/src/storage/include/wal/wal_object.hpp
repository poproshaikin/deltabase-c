#pragma once 

#include "../shared.hpp"
#include <concepts>
#include <cstdint>
#include <variant>
#include <string>
#include <filesystem>

namespace storage {
    enum class wal_record_type : uint64_t {
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
        concept WithWalType_c = requires(const T x) 
        {
            T::type;
            x.lsn;
            x.serialize();
            x.estimate_size();

            requires std::same_as<decltype(T::type), const wal_record_type>;
            requires std::same_as<decltype(x.lsn), uint64_t>;
            requires std::same_as<decltype(x.serialize()), bytes_v>;
            requires std::same_as<decltype(x.estimate_size()), uint64_t>;
        };

        template <WithWalType_c... Ts> 
        using wal_record_variant = std::variant<Ts...>;
    };

    struct insert_record {
        static constexpr wal_record_type type = wal_record_type::INSERT;

        uint64_t lsn;
        std::string table_id;
        bytes_v serialized_row;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct create_schema_record {
        static constexpr wal_record_type type = wal_record_type::CREATE_SCHEMA; 

        uint64_t lsn;
        std::string schema_id;
        bytes_v serialized_schema;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct drop_schema_record {
        static constexpr wal_record_type type = wal_record_type::DROP_SCHEMA;

        uint64_t lsn;
        std::string schema_id;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    struct create_table_record {
        static constexpr wal_record_type type = wal_record_type::CREATE_TABLE;

        uint64_t lsn;
        std::string table_id;
        bytes_v serialized_table;

        bytes_v
        serialize() const;
        uint64_t
        estimate_size() const;
    };

    using WalRecord = detail::wal_record_variant<
        insert_record, 
        create_schema_record,   
        drop_schema_record,
        create_table_record
    >;

    struct wal_checkpoint {
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
        can_deserialize(bytes_v content) noexcept;

        static WalLogfile
        deserialize(bytes_v content);
        bytes_v
        serialize();
    };
}