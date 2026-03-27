//
// Created by poproshaikin on 27.11.25.
//

#ifndef DELTABASE_WAL_LOG_HPP
#define DELTABASE_WAL_LOG_HPP
#include "data_page.hpp"
#include "data_row.hpp"
#include "meta_schema.hpp"
#include "meta_table.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace types
{
    using LSN = uint64_t;

    enum class WALRecordType : uint64_t
    {
        INSERT = 1,
        UPDATE,
        DELETE,
        CREATE_TABLE,
        CREATE_SCHEMA,
        BEGIN_TXN,
        COMMIT_TXN,
        ROLLBACK_TXN
    };

    namespace detail
    {
        template <typename T>
        concept WAL_c = requires(const T x) {
            T::type;
            x.lsn;
            x.prev_lsn;
            x.txn_id;

            requires std::same_as<decltype(T::type), const WALRecordType>;
            requires std::same_as<decltype(x.lsn), LSN>;
            requires std::same_as<decltype(x.prev_lsn), LSN>;
            requires std::same_as<decltype(x.txn_id), Uuid>;
        };

        template <WAL_c... Ts>
        using WALRecordVariant = std::variant<Ts...>;

        template <typename T>
        concept HasPageId_c = requires(const T& x) {
            x.page_id;

            requires std::same_as<decltype(x.page_id), PageId>;
        };
    }; // namespace detail

    struct InsertRecord
    {
        static constexpr auto type = WALRecordType::INSERT;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        DataRow after;

        InsertRecord() = delete;

        InsertRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id),
              page_id(page_id), after(std::move(after))
        {
        }
    };

    struct UpdateRecord
    {
        static constexpr auto type = WALRecordType::UPDATE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        DataRow before;
        DataRow after;

        UpdateRecord() = delete;

        UpdateRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow before,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id),
              page_id(page_id), before(std::move(before)), after(std::move(after))
        {
        }
    };

    struct DeleteRecord
    {
        static constexpr auto type = WALRecordType::DELETE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        DataRow before;

        DeleteRecord() = delete;

        DeleteRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow before
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id),
              page_id(page_id), before(std::move(before))
        {
        }
    };

    struct CreateSchemaRecord
    {
        static constexpr auto type = WALRecordType::CREATE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaSchema schema;

        CreateSchemaRecord() = delete;

        CreateSchemaRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaSchema schema)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), schema(std::move(schema))
        {
        }
    };

    struct CreateTableRecord
    {
        static constexpr auto type = WALRecordType::CREATE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaTable table;

        CreateTableRecord() = delete;

        CreateTableRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaTable table)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table(std::move(table))
        {
        }
    };

    struct BeginTxnRecord
    {
        static constexpr auto type = WALRecordType::BEGIN_TXN;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;

        BeginTxnRecord() = delete;

        BeginTxnRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id)
        {
        }
    };

    struct CommitTxnRecord
    {
        static constexpr auto type = WALRecordType::COMMIT_TXN;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;

        CommitTxnRecord() = delete;

        CommitTxnRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id)
        {
        }
    };

    struct RollbackTxnRecord
    {
        static constexpr auto type = WALRecordType::ROLLBACK_TXN;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;

        RollbackTxnRecord() = delete;

        RollbackTxnRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id)
        {
        }
    };

    using WALRecord = detail::WALRecordVariant<
        InsertRecord,
        UpdateRecord,
        DeleteRecord,
        CreateSchemaRecord,
        CreateTableRecord,
        BeginTxnRecord,
        CommitTxnRecord,
        RollbackTxnRecord>;

    using WALDataRecord = detail::WALRecordVariant<InsertRecord, UpdateRecord, DeleteRecord>;
    using WALMetaRecord = detail::WALRecordVariant<CreateSchemaRecord, CreateTableRecord>;
    using WALTxnRecord = detail::WALRecordVariant<BeginTxnRecord, CommitTxnRecord, RollbackTxnRecord>;

    template <typename T, typename Variant>
    struct is_in_variant;

    template <typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {};

    template <typename T, typename Variant>
    inline constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

    namespace wal_log
    {
        template <typename T> constexpr bool has_page_id_v = detail::HasPageId_c<T>;

        inline bool
        has_page_id(const WALRecord& record)
        {
            return std::visit(
                []<typename TRecord>(const TRecord& rec) -> bool
                { return has_page_id_v<std::decay_t<TRecord>>; },
                record
            );
        }

        inline LSN
        extract_lsn(const WALRecord& record)
        {
            return std::visit([](const auto& rec) { return rec.lsn; }, record);
        }

        inline PageId
        extract_page_id(const WALRecord& record)
        {
            return std::visit(
                []<typename TRecord>(const TRecord& rec) -> PageId {
                    if constexpr (
                        std::is_same_v<std::decay_t<TRecord>, InsertRecord> ||
                        std::is_same_v<std::decay_t<TRecord>, UpdateRecord> ||
                        std::is_same_v<std::decay_t<TRecord>, DeleteRecord>)
                    {
                        return rec.page_id;
                    }

                    throw std::runtime_error("extract_page_id: record does not have page id");
                }, record
            );
        }
    } // namespace wal_log
} // namespace types

#endif // DELTABASE_WAL_LOG_HPP