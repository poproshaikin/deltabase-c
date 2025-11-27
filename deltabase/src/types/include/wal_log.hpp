//
// Created by poproshaikin on 27.11.25.
//

#ifndef DELTABASE_WAL_LOG_HPP
#define DELTABASE_WAL_LOG_HPP
#include "data_row.hpp"
#include "meta_schema.hpp"
#include "typedefs.hpp"

#include <cstdint>
#include <string>
#include <variant>

namespace types
{
    using Lsn = uint64_t;

    enum class WalRecordType : uint64_t
    {
        INSERT = 1,
        UPDATE_BY_FILTER,
        DELETE_BY_FILTER,
        CREATE_TABLE,
        CREATE_SCHEMA,
        DROP_TABLE,
        DROP_SCHEMA,
        BEGIN_TRANSACTION
    };

    namespace detail
    {
        template <typename T>
        concept Wal_c = requires(const T x, const Bytes& content, T& out) {
            T::type;
            x.lsn;

            requires std::same_as<decltype(T::type), const WalRecordType>;
            requires std::same_as<decltype(x.lsn), uint64_t>;
        };

        template <Wal_c... Ts>
        using WalRecordVariant = std::variant<Ts...>;
    };

    struct InsertRecord
    {
        static constexpr auto type = WalRecordType::INSERT;

        Lsn lsn;
        Uuid txn_id;
        Uuid table_id;
        DataRow row;
    };

    struct CreateSchemaRecord
    {
        static constexpr auto type = WalRecordType::CREATE_SCHEMA;

        Lsn lsn;
        Uuid txn_id;
        Uuid schema_id;
        MetaSchema schema;
    };

    struct DropSchemaRecord
    {
        static constexpr auto type = WalRecordType::DROP_SCHEMA;

        Lsn lsn;
        Uuid txn_id;
        Uuid schema_id;
    };

    struct CreateTableRecord
    {
        static constexpr auto type = WalRecordType::CREATE_TABLE;

        Lsn lsn;
        Uuid txn_id;
        Uuid table_id;
        MetaTable table;
    };

    struct BeginTransactionRecord
    {
        static constexpr auto type = WalRecordType::BEGIN_TRANSACTION;

        Lsn lsn;
        Uuid txn_id;
    };

    using WalRecord = detail::WalRecordVariant<
        InsertRecord,
        CreateSchemaRecord,
        DropSchemaRecord,
        CreateTableRecord,
        BeginTransactionRecord
    >;
}

#endif //DELTABASE_WAL_LOG_HPP