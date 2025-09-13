#pragma once 

#include "../shared.hpp"
#include <concepts>
#include <cstdint>
#include <variant>
#include <string>

namespace storage {
    enum class WalRecordType : uint64_t {
        INSERT = 1,
        UPDATE_BY_FILTER,
        DELETE_BY_FILTER,
        CREATE_META_TABLE,
        CREATE_META_SCHEMA
    }; 

    namespace detail {
        template <typename T>
        concept with_wal_type = requires(const T x) {
            x.type;
            x.serialize();
            requires std::same_as<decltype(x.type), const WalRecordType>;
            requires std::same_as<decltype(x.serialize()), bytes_arr>;
        };

        template <with_wal_type... Ts> 
        using WalRecordVariantTemplate = std::variant<Ts...>;
    };

    struct InsertRecord {
        const WalRecordType type = WalRecordType::INSERT;
        std::string table_id;
        bytes_arr serialized_row;

        bytes_arr
        serialize() const;
    };

    using WalRecord = detail::WalRecordVariantTemplate<InsertRecord>;
}