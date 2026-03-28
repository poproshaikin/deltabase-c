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
        UPDATE_TABLE,
        DELETE_TABLE,
        CREATE_SCHEMA,
        UPDATE_SCHEMA,
        DELETE_SCHEMA,
        BEGIN_TXN,
        COMMIT_TXN,
        ROLLBACK_TXN,
        CLR_INSERT,
        CLR_UPDATE,
        CLR_DELETE,
        CLR_CREATE_SCHEMA,
        CLR_UPDATE_SCHEMA,
        CLR_DELETE_SCHEMA,
        CLR_CREATE_TABLE,
        CLR_UPDATE_TABLE,
        CLR_DELETE_TABLE
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

        template <WAL_c... Ts> using WALRecordVariant = std::variant<Ts...>;

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

        InsertRecord() = default;

        InsertRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              after(std::move(after))
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

        UpdateRecord() = default;

        UpdateRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow before,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              before(std::move(before)), after(std::move(after))
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

        DeleteRecord() = default;

        DeleteRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            DataRow before
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              before(std::move(before))
        {
        }
    };

    struct CLRInsertRecord
    {
        static constexpr auto type = WALRecordType::CLR_INSERT;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        LSN undo_next_lsn;
        DataRow after;

        CLRInsertRecord() = default;

        CLRInsertRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            LSN undo_next_lsn,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              undo_next_lsn(undo_next_lsn), after(std::move(after))
        {
        }
    };

    struct CLRUpdateRecord
    {
        static constexpr auto type = WALRecordType::CLR_UPDATE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        LSN undo_next_lsn;
        DataRow before;
        DataRow after;

        CLRUpdateRecord() = default;

        CLRUpdateRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            LSN undo_next_lsn,
            DataRow before,
            DataRow after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              undo_next_lsn(undo_next_lsn), before(std::move(before)), after(std::move(after))
        {
        }
    };

    struct CLRDeleteRecord
    {
        static constexpr auto type = WALRecordType::CLR_DELETE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        Uuid table_id;
        PageId page_id;
        LSN undo_next_lsn;
        DataRow before;

        CLRDeleteRecord() = default;

        CLRDeleteRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            const Uuid& table_id,
            const PageId& page_id,
            LSN undo_next_lsn,
            DataRow before
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), table_id(table_id), page_id(page_id),
              undo_next_lsn(undo_next_lsn), before(std::move(before))
        {
        }
    };

    struct CLRCreateSchemaRecord
    {
        static constexpr auto type = WALRecordType::CLR_CREATE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaSchema schema;

        CLRCreateSchemaRecord() = default;

        CLRCreateSchemaRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaSchema schema
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              schema(std::move(schema))
        {
        }
    };

    struct CLRUpdateSchemaRecord
    {
        static constexpr auto type = WALRecordType::CLR_UPDATE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaSchema before;
        MetaSchema after;

        CLRUpdateSchemaRecord() = default;

        CLRUpdateSchemaRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaSchema before,
            MetaSchema after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              before(std::move(before)), after(std::move(after))
        {
        }
    };

    struct CLRDeleteSchemaRecord
    {
        static constexpr auto type = WALRecordType::CLR_DELETE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaSchema before;

        CLRDeleteSchemaRecord() = default;

        CLRDeleteSchemaRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaSchema before
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              before(std::move(before))
        {
        }
    };

    struct CLRCreateTableRecord
    {
        static constexpr auto type = WALRecordType::CLR_CREATE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaTable after;

        CLRCreateTableRecord() = default;

        CLRCreateTableRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaTable after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              after(std::move(after))
        {
        }
    };

    struct CLRUpdateTableRecord
    {
        static constexpr auto type = WALRecordType::CLR_UPDATE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaTable before;
        MetaTable after;

        CLRUpdateTableRecord() = default;

        CLRUpdateTableRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaTable before,
            MetaTable after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              before(std::move(before)), after(std::move(after))
        {
        }
    };

    struct CLRDeleteTableRecord
    {
        static constexpr auto type = WALRecordType::CLR_DELETE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        LSN undo_next_lsn;
        MetaTable before;

        CLRDeleteTableRecord() = default;

        CLRDeleteTableRecord(
            LSN lsn,
            LSN prev_lsn,
            const Uuid& txn_id,
            LSN undo_next_lsn,
            MetaTable before
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), undo_next_lsn(undo_next_lsn),
              before(std::move(before))
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

        CreateSchemaRecord() = default;

        CreateSchemaRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaSchema schema)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), schema(std::move(schema))
        {
        }
    };

    struct UpdateSchemaRecord
    {
        static constexpr auto type = WALRecordType::UPDATE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaSchema before;
        MetaSchema after;

        UpdateSchemaRecord() = default;

        UpdateSchemaRecord(
            LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaSchema before, MetaSchema after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), before(std::move(before)),
              after(std::move(after))
        {
        }
    };

    struct DeleteSchemaRecord
    {
        static constexpr auto type = WALRecordType::DELETE_SCHEMA;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaSchema before;

        DeleteSchemaRecord() = default;

        DeleteSchemaRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaSchema before)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), before(std::move(before))
        {
        }
    };

    struct CreateTableRecord
    {
        static constexpr auto type = WALRecordType::CREATE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaTable after;

        CreateTableRecord() = default;

        CreateTableRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaTable table)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), after(std::move(table))
        {
        }
    };

    struct UpdateTableRecord
    {
        static constexpr auto type = WALRecordType::UPDATE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaTable before;
        MetaTable after;

        UpdateTableRecord() = default;

        UpdateTableRecord(
            LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaTable before, MetaTable after
        )
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), before(std::move(before)),
              after(std::move(after))
        {
        }
    };

    struct DeleteTableRecord
    {
        static constexpr auto type = WALRecordType::DELETE_TABLE;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;
        MetaTable before;

        DeleteTableRecord() = default;

        DeleteTableRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id, MetaTable before)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id), before(std::move(before))
        {
        }
    };

    struct BeginTxnRecord
    {
        static constexpr auto type = WALRecordType::BEGIN_TXN;

        LSN lsn;
        LSN prev_lsn;
        Uuid txn_id;

        BeginTxnRecord() = default;

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

        CommitTxnRecord() = default;

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

        RollbackTxnRecord() = default;

        RollbackTxnRecord(LSN lsn, LSN prev_lsn, const Uuid& txn_id)
            : lsn(lsn), prev_lsn(prev_lsn), txn_id(txn_id)
        {
        }
    };

    using WALRecord = detail::WALRecordVariant<
        InsertRecord,
        UpdateRecord,
        DeleteRecord,
        CLRInsertRecord,
        CLRUpdateRecord,
        CLRDeleteRecord,
        CLRCreateSchemaRecord,
        CLRUpdateSchemaRecord,
        CLRDeleteSchemaRecord,
        CLRCreateTableRecord,
        CLRUpdateTableRecord,
        CLRDeleteTableRecord,
        CreateSchemaRecord,
        UpdateSchemaRecord,
        DeleteSchemaRecord,
        CreateTableRecord,
        UpdateTableRecord,
        DeleteTableRecord,
        BeginTxnRecord,
        CommitTxnRecord,
        RollbackTxnRecord>;

    using WALDataRecord = detail::WALRecordVariant<
        InsertRecord,
        UpdateRecord,
        DeleteRecord,
        CLRInsertRecord,
        CLRUpdateRecord,
        CLRDeleteRecord>;

    using WALMetaRecord = detail::WALRecordVariant<
        CreateSchemaRecord,
        UpdateSchemaRecord,
        DeleteSchemaRecord,
        CreateTableRecord,
        UpdateTableRecord,
        DeleteTableRecord>;

    using WALMetaCLRRecord = detail::WALRecordVariant<
        CLRCreateSchemaRecord,
        CLRUpdateSchemaRecord,
        CLRDeleteSchemaRecord,
        CLRCreateTableRecord,
        CLRUpdateTableRecord,
        CLRDeleteTableRecord>;

    using WALCLRRecord = detail::WALRecordVariant<
        CLRInsertRecord,
        CLRUpdateRecord,
        CLRDeleteRecord,
        CLRCreateSchemaRecord,
        CLRUpdateSchemaRecord,
        CLRDeleteSchemaRecord,
        CLRCreateTableRecord,
        CLRUpdateTableRecord,
        CLRDeleteTableRecord>;

    using WALTxnRecord =
        detail::WALRecordVariant<BeginTxnRecord, CommitTxnRecord, RollbackTxnRecord>;

    template <typename T, typename Variant> struct is_in_variant;

    template <typename T, typename... Ts>
    struct is_in_variant<T, std::variant<Ts...>> : std::disjunction<std::is_same<T, Ts>...>
    {
    };

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
                []<typename TRecord>(const TRecord& rec) -> PageId
                {
                    if constexpr (std::is_same_v<std::decay_t<TRecord>, InsertRecord> ||
                                  std::is_same_v<std::decay_t<TRecord>, UpdateRecord> ||
                                  std::is_same_v<std::decay_t<TRecord>, DeleteRecord>)
                    {
                        return rec.page_id;
                    }

                    throw std::runtime_error("extract_page_id: record does not have page id");
                },
                record
            );
        }
    } // namespace wal_log
} // namespace types

#endif // DELTABASE_WAL_LOG_HPP