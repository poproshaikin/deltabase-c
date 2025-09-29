#pragma once 

#include "../shared.hpp"
#include <concepts>
#include <cstdint>
#include <variant>
#include <string>

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
        concept WithWalType_c = requires(const T x) {
            x.serialize();
            T::type;
            requires std::same_as<decltype(T::type), const wal_record_type>;
            requires std::same_as<decltype(x.serialize()), bytes_v>;
        };

        template <WithWalType_c... Ts> 
        using WalRecordVariantTemplate = std::variant<Ts...>;
    };

    struct insert_record {
        static constexpr wal_record_type type = wal_record_type::INSERT;

        std::string table_id;
        bytes_v serialized_row;

        bytes_v
        serialize() const;
    };

    struct create_schema_record {
        static constexpr wal_record_type type = wal_record_type::CREATE_SCHEMA; 

        std::string schema_id;
        bytes_v serialized_schema;

        bytes_v
        serialize() const;
    };

    struct drop_schema_record {
        static constexpr wal_record_type type = wal_record_type::DROP_SCHEMA;

        std::string schema_id;
        bytes_v
        serialize() const;
    };

    struct create_table_record {
        static constexpr wal_record_type type = wal_record_type::CREATE_TABLE;

        std::string table_id;
        bytes_v serialized_table;
        bytes_v
        serialize() const;
    };

    using WalRecord = detail::WalRecordVariantTemplate<
        insert_record, 
        create_schema_record,   
        drop_schema_record,
        create_table_record
    >;

    struct wal_checkpoint {
        uint64_t lsn;
    };
}