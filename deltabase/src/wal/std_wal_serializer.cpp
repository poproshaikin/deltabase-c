//
// Created by poproshaikin on 04.03.26.
//

#include "include/std_wal_serializer.hpp"
namespace wal
{
    using namespace types;
    using namespace misc;

    MemoryStream
    StdWalSerializer::serialize(const BeginTxnRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const InsertRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));

        auto serialized_row = binary_serializer_.serialize_dr(record.after);
        stream.append(serialized_row, serialized_row.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));

        // Сериализуем old_row
        auto serialized_old_row = binary_serializer_.serialize_dr(record.before);
        stream.append(serialized_old_row, serialized_old_row.size());

        // Сериализуем new_row
        auto serialized_new_row = binary_serializer_.serialize_dr(record.after);
        stream.append(serialized_new_row, serialized_new_row.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));

        auto serialized_row = binary_serializer_.serialize_dr(record.before);
        stream.append(serialized_row, serialized_row.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRInsertRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_row = binary_serializer_.serialize_dr(record.after);
        stream.append(serialized_row, serialized_row.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_dr(record.before);
        stream.append(serialized_before, serialized_before.size());

        auto serialized_after = binary_serializer_.serialize_dr(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));
        stream.write(&record.page_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_dr(record.before);
        stream.append(serialized_before, serialized_before.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_schema = binary_serializer_.serialize_ms(record.schema);
        stream.append(serialized_schema, serialized_schema.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_ms(record.before);
        stream.append(serialized_before, serialized_before.size());

        auto serialized_after = binary_serializer_.serialize_ms(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_ms(record.before);
        stream.append(serialized_before, serialized_before.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_after = binary_serializer_.serialize_mt(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRUpdateTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_mt(record.before);
        stream.append(serialized_before, serialized_before.size());

        auto serialized_after = binary_serializer_.serialize_mt(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDeleteTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_before = binary_serializer_.serialize_mt(record.before);
        stream.append(serialized_before, serialized_before.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_schema = binary_serializer_.serialize_ms(record.schema);
        stream.append(serialized_schema, serialized_schema.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_before = binary_serializer_.serialize_ms(record.before);
        stream.append(serialized_before, serialized_before.size());

        auto serialized_after = binary_serializer_.serialize_ms(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_before = binary_serializer_.serialize_ms(record.before);
        stream.append(serialized_before, serialized_before.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_table = binary_serializer_.serialize_mt(record.after);
        stream.append(serialized_table, serialized_table.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const UpdateTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_before = binary_serializer_.serialize_mt(record.before);
        stream.append(serialized_before, serialized_before.size());

        auto serialized_after = binary_serializer_.serialize_mt(record.after);
        stream.append(serialized_after, serialized_after.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const DeleteTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_before = binary_serializer_.serialize_mt(record.before);
        stream.append(serialized_before, serialized_before.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateIndexRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_index = binary_serializer_.serialize_mi(record.after);
        stream.append(serialized_index, serialized_index.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRCreateIndexRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_index = binary_serializer_.serialize_mi(record.after);
        stream.append(serialized_index, serialized_index.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const DropIndexRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

        auto serialized_index = binary_serializer_.serialize_mi(record.before);
        stream.append(serialized_index, serialized_index.size());
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CLRDropIndexRecord& record) const
    {
        MemoryStream stream;

        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.undo_next_lsn, sizeof(record.undo_next_lsn));

        auto serialized_index = binary_serializer_.serialize_mi(record.before);
        stream.append(serialized_index, serialized_index.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CommitTxnRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const RollbackTxnRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.prev_lsn, sizeof(record.prev_lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
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
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            out = BeginTxnRecord(lsn, prev_lsn, txn_id);
            return true;
        }

        case WALRecordType::COMMIT_TXN:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            out = CommitTxnRecord(lsn, prev_lsn, txn_id);
            return true;
        }

        case WALRecordType::ROLLBACK_TXN:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            out = RollbackTxnRecord(lsn, prev_lsn, txn_id);
            return true;
        }

        case WALRecordType::INSERT:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            DataRow after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_dr(stream, after))
                return false;

            out = InsertRecord(lsn, prev_lsn, txn_id, table_id, page_id, std::move(after));
            return true;
        }

        case WALRecordType::UPDATE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            DataRow before;
            DataRow after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_dr(stream, before))
                return false;

            if (!binary_serializer_.deserialize_dr(stream, after))
                return false;

            out = UpdateRecord(
                lsn, prev_lsn, txn_id, table_id, page_id, std::move(before), std::move(after)
            );
            return true;
        }

        case WALRecordType::DELETE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            DataRow before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_dr(stream, before))
                return false;

            out = DeleteRecord(lsn, prev_lsn, txn_id, table_id, page_id, std::move(before));
            return true;
        }

        case WALRecordType::CREATE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaSchema schema;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_ms(stream, schema))
                return false;

            out = CreateSchemaRecord(lsn, prev_lsn, txn_id, std::move(schema));
            return true;
        }

        case WALRecordType::UPDATE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaSchema before;
            MetaSchema after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_ms(stream, before))
                return false;
            if (!binary_serializer_.deserialize_ms(stream, after))
                return false;

            out = UpdateSchemaRecord(lsn, prev_lsn, txn_id, std::move(before), std::move(after));
            return true;
        }

        case WALRecordType::DELETE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaSchema before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_ms(stream, before))
                return false;

            out = DeleteSchemaRecord(lsn, prev_lsn, txn_id, std::move(before));
            return true;
        }

        case WALRecordType::CREATE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaTable table;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_mt(stream, table))
                return false;

            out = CreateTableRecord(lsn, prev_lsn, txn_id, std::move(table));
            return true;
        }

        case WALRecordType::UPDATE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaTable before;
            MetaTable after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_mt(stream, before))
                return false;
            if (!binary_serializer_.deserialize_mt(stream, after))
                return false;

            out = UpdateTableRecord(lsn, prev_lsn, txn_id, std::move(before), std::move(after));
            return true;
        }

        case WALRecordType::DELETE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaTable before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;

            if (!binary_serializer_.deserialize_mt(stream, before))
                return false;

            out = DeleteTableRecord(lsn, prev_lsn, txn_id, std::move(before));
            return true;
        }

        case WALRecordType::CREATE_INDEX:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaIndex after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!binary_serializer_.deserialize_mi(stream, after))
                return false;

            out = CreateIndexRecord(lsn, prev_lsn, txn_id, after);
            return true;
        }

        case WALRecordType::CLR_INSERT:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            LSN undo_next_lsn;
            DataRow after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_dr(stream, after))
                return false;

            out = CLRInsertRecord(
                lsn, prev_lsn, txn_id, table_id, page_id, undo_next_lsn, std::move(after)
            );
            return true;
        }

        case WALRecordType::CLR_UPDATE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            LSN undo_next_lsn;
            DataRow before;
            DataRow after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_dr(stream, before))
                return false;
            if (!binary_serializer_.deserialize_dr(stream, after))
                return false;

            out = CLRUpdateRecord(
                lsn,
                prev_lsn,
                txn_id,
                table_id,
                page_id,
                undo_next_lsn,
                std::move(before),
                std::move(after)
            );
            return true;
        }

        case WALRecordType::CLR_DELETE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            Uuid table_id;
            Uuid page_id;
            LSN undo_next_lsn;
            DataRow before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(page_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_dr(stream, before))
                return false;

            out = CLRDeleteRecord(
                lsn, prev_lsn, txn_id, table_id, page_id, undo_next_lsn, std::move(before)
            );
            return true;
        }

        case WALRecordType::CLR_CREATE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaSchema schema;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_ms(stream, schema))
                return false;

            out = CLRCreateSchemaRecord(lsn, prev_lsn, txn_id, undo_next_lsn, std::move(schema));
            return true;
        }

        case WALRecordType::CLR_UPDATE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaSchema before;
            MetaSchema after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_ms(stream, before))
                return false;
            if (!binary_serializer_.deserialize_ms(stream, after))
                return false;

            out = CLRUpdateSchemaRecord(
                lsn, prev_lsn, txn_id, undo_next_lsn, std::move(before), std::move(after)
            );
            return true;
        }

        case WALRecordType::CLR_DELETE_SCHEMA:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaSchema before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_ms(stream, before))
                return false;

            out = CLRDeleteSchemaRecord(lsn, prev_lsn, txn_id, undo_next_lsn, std::move(before));
            return true;
        }

        case WALRecordType::CLR_CREATE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaTable after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_mt(stream, after))
                return false;

            out = CLRCreateTableRecord(lsn, prev_lsn, txn_id, undo_next_lsn, std::move(after));
            return true;
        }

        case WALRecordType::CLR_UPDATE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaTable before;
            MetaTable after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_mt(stream, before))
                return false;
            if (!binary_serializer_.deserialize_mt(stream, after))
                return false;

            out = CLRUpdateTableRecord(
                lsn, prev_lsn, txn_id, undo_next_lsn, std::move(before), std::move(after)
            );
            return true;
        }

        case WALRecordType::CLR_DELETE_TABLE:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaTable before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_mt(stream, before))
                return false;

            out = CLRDeleteTableRecord(lsn, prev_lsn, txn_id, undo_next_lsn, std::move(before));
            return true;
        }

        case WALRecordType::CLR_CREATE_INDEX:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaIndex after;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_mi(stream, after))
                return false;

            out = CLRCreateIndexRecord(lsn, prev_lsn, txn_id, undo_next_lsn, after);
            return true;
        }

        case WALRecordType::DROP_INDEX:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            MetaIndex before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!binary_serializer_.deserialize_mi(stream, before))
                return false;

            out = DropIndexRecord(lsn, prev_lsn, txn_id, std::move(before));
            return true;
        }

        case WALRecordType::CLR_DROP_INDEX:
        {
            LSN lsn;
            LSN prev_lsn;
            Uuid txn_id;
            LSN undo_next_lsn;
            MetaIndex before;

            if (!stream.read(&lsn, sizeof(lsn)))
                return false;
            if (!stream.read(&prev_lsn, sizeof(prev_lsn)))
                return false;
            if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                return false;
            if (!stream.read(&undo_next_lsn, sizeof(undo_next_lsn)))
                return false;
            if (!binary_serializer_.deserialize_mi(stream, before))
                return false;

            out = CLRDropIndexRecord(lsn, prev_lsn, txn_id, undo_next_lsn, std::move(before));
            return true;
        }

        default:
            return false;
        }
    }
} // namespace wal
