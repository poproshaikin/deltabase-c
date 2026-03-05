//
// Created by poproshaikin on 27.11.25.
//

#ifndef DELTABASE_WAL_LOG_HPP
#define DELTABASE_WAL_LOG_HPP
#include "data_row.hpp"
#include "meta_schema.hpp"
#include "meta_table.hpp"
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
        UPDATE,
        DELETE,
        CREATE_TABLE,
        CREATE_SCHEMA,
        BEGIN_TRANSACTION,
        COMMIT_TRANSACTION
    };

    namespace detail
    {
        template <typename T>
        concept Wal_c = requires(const T x) {
            T::type;
            x.lsn;

            requires std::same_as<decltype(T::type), const WalRecordType>;
            requires std::same_as<decltype(x.lsn), uint64_t>;
        };

        template <Wal_c... Ts> using WalRecordVariant = std::variant<Ts...>;
    }; // namespace detail

    struct InsertRecord
    {
        static constexpr auto type = WalRecordType::INSERT;

        Lsn lsn;
        Uuid txn_id;
        Uuid table_id;
        DataRow row;
    };

    struct UpdateRecord
    {
        static constexpr auto type = WalRecordType::UPDATE;

        Lsn lsn;
        Uuid txn_id;

        Uuid table_id;

        DataRow old_row;
        DataRow new_row;
    };

    struct DeleteRecord
    {
        static constexpr auto type = WalRecordType::DELETE;

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
        MetaSchema schema;
    };

    struct CreateTableRecord
    {
        static constexpr auto type = WalRecordType::CREATE_TABLE;

        Lsn lsn;
        Uuid txn_id;
        MetaTable table;
    };

    struct BeginTransactionRecord
    {
        static constexpr auto type = WalRecordType::BEGIN_TRANSACTION;

        Lsn lsn;
        Uuid txn_id;
    };

    struct CommitTransactionRecord
    {
        static constexpr auto type = WalRecordType::COMMIT_TRANSACTION;

        Lsn lsn;
        Uuid txn_id;
    };

    using WalRecord = detail::WalRecordVariant<
        InsertRecord,
        UpdateRecord,
        DeleteRecord,
        CreateSchemaRecord,
        CreateTableRecord,
        BeginTransactionRecord,
        CommitTransactionRecord>;
} // namespace types

#endif // DELTABASE_WAL_LOG_HPP