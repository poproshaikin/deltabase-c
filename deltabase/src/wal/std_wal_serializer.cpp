//
// Created by poproshaikin on 04.03.26.
//

#include "include/std_wal_serializer.hpp"
#include "../misc/include/serialization/schema_fields.hpp"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace
{
    using namespace misc::serialization;
    using namespace types;

    template <typename TNested>
    using SerializeNestedFn = misc::MemoryStream (storage::StdBinarySerializer::*)(const TNested&) const;

    template <typename TNested>
    using DeserializeNestedFn =
        bool (storage::StdBinarySerializer::*)(misc::ReadOnlyMemoryStream&, TNested&) const;

    template <typename TObject>
    misc::MemoryStream
    serialize_object(const TObject& object, const Schema<TObject>& schema, const char* error)
    {
        misc::MemoryStream stream;
        WriteCursor cursor(stream);
        if (!write_by_schema(object, schema, cursor))
            throw std::runtime_error(error);

        stream.seek(0);
        return stream;
    }

    template <typename TObject>
    bool
    deserialize_object(misc::ReadOnlyMemoryStream& stream, const Schema<TObject>& schema, TObject& out)
    {
        ReadCursor cursor(stream);
        return read_by_schema(cursor, schema, out);
    }

    template <typename TRecord>
    void
    add_wal_header_fields(Schema<TRecord>& schema)
    {
        schema.fields.push_back(make_pod_field(&TRecord::lsn));
        schema.fields.push_back(make_pod_field(&TRecord::prev_lsn));
        schema.fields.push_back(make_uuid_field(&TRecord::txn_id));
    }

    template <typename TRecord>
    void
    add_page_context_fields(Schema<TRecord>& schema)
    {
        schema.fields.push_back(make_uuid_field(&TRecord::table_id));
        schema.fields.push_back(make_uuid_field(&TRecord::page_id));
    }

    template <typename TRecord>
    void
    add_undo_next_lsn_field(Schema<TRecord>& schema)
    {
        schema.fields.push_back(make_pod_field(&TRecord::undo_next_lsn));
    }

    template <typename TRecord, typename TNested>
    std::unique_ptr<ISchemaField<TRecord>>
    make_serialized_member_field(
        const storage::StdBinarySerializer* binary_serializer,
        TNested TRecord::* member_ptr,
        SerializeNestedFn<TNested> serialize_fn,
        DeserializeNestedFn<TNested> deserialize_fn
    )
    {
        return make_custom_field<TRecord>(
            [binary_serializer, member_ptr, serialize_fn](const TRecord& record, WriteCursor& cursor)
            {
                const auto serialized = (binary_serializer->*serialize_fn)(record.*member_ptr);
                return cursor.write_bytes(serialized.data(), serialized.size());
            },
            [binary_serializer, member_ptr, deserialize_fn](ReadCursor& cursor, TRecord& record)
            {
                return (binary_serializer->*deserialize_fn)(cursor.stream(), record.*member_ptr);
            }
        );
    }

    template <typename TRecord, typename TNested>
    std::unique_ptr<ISchemaField<TRecord>>
    make_serialized_member_pair_field(
        const storage::StdBinarySerializer* binary_serializer,
        TNested TRecord::* before_member,
        TNested TRecord::* after_member,
        SerializeNestedFn<TNested> serialize_fn,
        DeserializeNestedFn<TNested> deserialize_fn
    )
    {
        return make_custom_field<TRecord>(
            [binary_serializer, before_member, after_member, serialize_fn](
                const TRecord& record,
                WriteCursor& cursor
            )
            {
                const auto serialized_before = (binary_serializer->*serialize_fn)(record.*before_member);
                if (!cursor.write_bytes(serialized_before.data(), serialized_before.size()))
                    return false;

                const auto serialized_after = (binary_serializer->*serialize_fn)(record.*after_member);
                return cursor.write_bytes(serialized_after.data(), serialized_after.size());
            },
            [binary_serializer, before_member, after_member, deserialize_fn](
                ReadCursor& cursor,
                TRecord& record
            )
            {
                if (!(binary_serializer->*deserialize_fn)(cursor.stream(), record.*before_member))
                    return false;

                return (binary_serializer->*deserialize_fn)(cursor.stream(), record.*after_member);
            }
        );
    }

    template <typename TRecord>
    Schema<TRecord>
    make_wal_schema(const storage::StdBinarySerializer* binary_serializer)
    {
        Schema<TRecord> schema;
        add_wal_header_fields(schema);

        if constexpr (std::is_same_v<TRecord, BeginTxnRecord> ||
                      std::is_same_v<TRecord, CommitTxnRecord> ||
                      std::is_same_v<TRecord, RollbackTxnRecord>)
        {
            return schema;
        }
        else if constexpr (std::is_same_v<TRecord, InsertRecord>)
        {
            add_page_context_fields(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, UpdateRecord>)
        {
            add_page_context_fields(schema);
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, DeleteRecord>)
        {
            add_page_context_fields(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRInsertRecord>)
        {
            add_page_context_fields(schema);
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRUpdateRecord>)
        {
            add_page_context_fields(schema);
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRDeleteRecord>)
        {
            add_page_context_fields(schema);
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_dr,
                &storage::StdBinarySerializer::deserialize_dr
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CreateSchemaRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::schema,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, UpdateSchemaRecord>)
        {
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, DeleteSchemaRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRCreateSchemaRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::schema,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRUpdateSchemaRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRDeleteSchemaRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_ms,
                &storage::StdBinarySerializer::deserialize_ms
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CreateTableRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, UpdateTableRecord>)
        {
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, DeleteTableRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRCreateTableRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRUpdateTableRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_pair_field(
                binary_serializer,
                &TRecord::before,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRDeleteTableRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_mt,
                &storage::StdBinarySerializer::deserialize_mt
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CreateIndexRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mi,
                &storage::StdBinarySerializer::deserialize_mi
            ));
        }
        else if constexpr (std::is_same_v<TRecord, DropIndexRecord>)
        {
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_mi,
                &storage::StdBinarySerializer::deserialize_mi
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRCreateIndexRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::after,
                &storage::StdBinarySerializer::serialize_mi,
                &storage::StdBinarySerializer::deserialize_mi
            ));
        }
        else if constexpr (std::is_same_v<TRecord, CLRDropIndexRecord>)
        {
            add_undo_next_lsn_field(schema);
            schema.fields.push_back(make_serialized_member_field(
                binary_serializer,
                &TRecord::before,
                &storage::StdBinarySerializer::serialize_mi,
                &storage::StdBinarySerializer::deserialize_mi
            ));
        }
        else
        {
            static_assert(sizeof(TRecord) == 0, "Unsupported WAL record type");
        }

        return schema;
    }

    template <typename TRecord>
    misc::MemoryStream
    serialize_record(const storage::StdBinarySerializer* binary_serializer, const TRecord& record)
    {
        auto payload = serialize_object(
            record,
            make_wal_schema<TRecord>(binary_serializer),
            "StdWalSerializer::serialize: schema write failed"
        );

        const auto type = TRecord::type;
        misc::MemoryStream out;
        out.write(&type, sizeof(type));
        out.write(payload.data(), payload.size());
        out.seek(0);
        return out;
    }

    template <typename TRecord>
    bool
    deserialize_record(
        const storage::StdBinarySerializer* binary_serializer,
        misc::ReadOnlyMemoryStream& stream,
        TRecord& out
    )
    {
        return deserialize_object(stream, make_wal_schema<TRecord>(binary_serializer), out);
    }

    template <typename TRecord>
    bool
    deserialize_to_variant(
        const storage::StdBinarySerializer* binary_serializer,
        misc::ReadOnlyMemoryStream& stream,
        WALRecord& out
    )
    {
        TRecord record;
        if (!deserialize_record(binary_serializer, stream, record))
            return false;

        out = std::move(record);
        return true;
    }
} // namespace

namespace wal
{
    using namespace types;
    using namespace misc;

    MemoryStream
    StdWalSerializer::serialize(const BeginTxnRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CommitTxnRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const RollbackTxnRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const InsertRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRInsertRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteSchemaRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteTableRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateIndexRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateIndexRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const DropIndexRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDropIndexRecord& record) const
    {
        return serialize_record(&binary_serializer_, record);
    }

    MemoryStream
    StdWalSerializer::serialize(const WALRecord& record) const
    {
        return std::visit([this](const auto& rec)
        {
            return serialize(rec);
        }, record);
    }

    bool
    StdWalSerializer::deserialize(ReadOnlyMemoryStream& stream, WALRecord& out)
    {
        WALRecordType type;
        if (!stream.read(&type, sizeof(type)))
            return false;

        switch (type)
        {
        case WALRecordType::BEGIN_TXN:
            return deserialize_to_variant<BeginTxnRecord>(&binary_serializer_, stream, out);

        case WALRecordType::COMMIT_TXN:
            return deserialize_to_variant<CommitTxnRecord>(&binary_serializer_, stream, out);

        case WALRecordType::ROLLBACK_TXN:
            return deserialize_to_variant<RollbackTxnRecord>(&binary_serializer_, stream, out);

        case WALRecordType::INSERT:
            return deserialize_to_variant<InsertRecord>(&binary_serializer_, stream, out);

        case WALRecordType::UPDATE:
            return deserialize_to_variant<UpdateRecord>(&binary_serializer_, stream, out);

        case WALRecordType::DELETE:
            return deserialize_to_variant<DeleteRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CREATE_SCHEMA:
            return deserialize_to_variant<CreateSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::UPDATE_SCHEMA:
            return deserialize_to_variant<UpdateSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::DELETE_SCHEMA:
            return deserialize_to_variant<DeleteSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_CREATE_SCHEMA:
            return deserialize_to_variant<CLRCreateSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_UPDATE_SCHEMA:
            return deserialize_to_variant<CLRUpdateSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_DELETE_SCHEMA:
            return deserialize_to_variant<CLRDeleteSchemaRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CREATE_TABLE:
            return deserialize_to_variant<CreateTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::UPDATE_TABLE:
            return deserialize_to_variant<UpdateTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::DELETE_TABLE:
            return deserialize_to_variant<DeleteTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_CREATE_TABLE:
            return deserialize_to_variant<CLRCreateTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_UPDATE_TABLE:
            return deserialize_to_variant<CLRUpdateTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_DELETE_TABLE:
            return deserialize_to_variant<CLRDeleteTableRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CREATE_INDEX:
            return deserialize_to_variant<CreateIndexRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_CREATE_INDEX:
            return deserialize_to_variant<CLRCreateIndexRecord>(&binary_serializer_, stream, out);

        case WALRecordType::DROP_INDEX:
            return deserialize_to_variant<DropIndexRecord>(&binary_serializer_, stream, out);

        case WALRecordType::CLR_DROP_INDEX:
            return deserialize_to_variant<CLRDropIndexRecord>(&binary_serializer_, stream, out);

        default:
            return false;
        }
    }
} // namespace wal
