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
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const InsertRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));

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
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));

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
        stream.write(&record.txn_id, sizeof(uuid_t));
        stream.write(&record.table_id, sizeof(uuid_t));

        auto serialized_row = binary_serializer_.serialize_dr(record.before);
        stream.append(serialized_row, serialized_row.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateSchemaRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

    auto serialized_schema = binary_serializer_.serialize_ms(record.schema);
        stream.append(serialized_schema, serialized_schema.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CreateTableRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));

    auto serialized_table = binary_serializer_.serialize_mt(record.table);
        stream.append(serialized_table, serialized_table.size());

        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const CommitTxnRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const WALRecord& record) const
    {
        switch (WALRecordType type = std::visit([](const auto& rec) { return rec.type; }, record))
        {
        case WALRecordType::INSERT:
            return serialize(std::get<InsertRecord>(record));
        case WALRecordType::UPDATE:
            return serialize(std::get<UpdateRecord>(record));
        case WALRecordType::DELETE:
            return serialize(std::get<DeleteRecord>(record));
        case WALRecordType::CREATE_TABLE:
            return serialize(std::get<CreateTableRecord>(record));
        case WALRecordType::CREATE_SCHEMA:
            return serialize(std::get<CreateSchemaRecord>(record));
        case WALRecordType::BEGIN_TXN:
            return serialize(std::get<BeginTxnRecord>(record));
        case WALRecordType::COMMIT_TXN:
            return serialize(std::get<CommitTxnRecord>(record));
        case WALRecordType::ROLLBACK_TXN:
            throw std::runtime_error("StdWalSerializer::serialize: rollback serialization is not implemented yet");
        default:
            throw std::runtime_error("StdWalSerializer::serialize: Unknown WAL record type " + std::to_string(static_cast<int>(type)));
        }
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
                Uuid txn_id;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                
                out = BeginTxnRecord(lsn, 0, txn_id);
                return true;
            }
            
        case WALRecordType::COMMIT_TXN:
            {
                LSN lsn;
                Uuid txn_id;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                
                out = CommitTxnRecord(lsn, 0, txn_id);
                return true;
            }

        case WALRecordType::ROLLBACK_TXN:
            {
                LSN lsn;
                Uuid txn_id;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;

                out = RollbackTxnRecord(lsn, 0, txn_id);
                return true;
            }
            
        case WALRecordType::INSERT:
            {
                LSN lsn;
                Uuid txn_id;
                Uuid table_id;
                DataRow after;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                    return false;
                
                if (!binary_serializer_.deserialize_dr(stream, after))
                    return false;
                
                out = InsertRecord(lsn, 0, txn_id, table_id, Uuid{}, std::move(after));
                return true;
            }
            
        case WALRecordType::UPDATE:
            {
                LSN lsn;
                Uuid txn_id;
                Uuid table_id;
                DataRow before;
                DataRow after;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                    return false;
                
                if (!binary_serializer_.deserialize_dr(stream, before))
                    return false;
                
                if (!binary_serializer_.deserialize_dr(stream, after))
                    return false;
                
                out = UpdateRecord(lsn, 0, txn_id, table_id, Uuid{}, std::move(before), std::move(after));
                return true;
            }
            
        case WALRecordType::DELETE:
            {
                LSN lsn;
                Uuid txn_id;
                Uuid table_id;
                DataRow before;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                if (!stream.read(table_id.raw(), sizeof(uuid_t)))
                    return false;
                
                if (!binary_serializer_.deserialize_dr(stream, before))
                    return false;
                
                out = DeleteRecord(lsn, 0, txn_id, table_id, Uuid{}, std::move(before));
                return true;
            }
            
        case WALRecordType::CREATE_SCHEMA:
            {
                LSN lsn;
                Uuid txn_id;
                MetaSchema schema;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                
                if (!binary_serializer_.deserialize_ms(stream, schema))
                    return false;
                
                out = CreateSchemaRecord(lsn, 0, txn_id, std::move(schema));
                return true;
            }
            
        case WALRecordType::CREATE_TABLE:
            {
                LSN lsn;
                Uuid txn_id;
                MetaTable table;

                if (!stream.read(&lsn, sizeof(lsn)))
                    return false;
                if (!stream.read(txn_id.raw(), sizeof(uuid_t)))
                    return false;
                
                if (!binary_serializer_.deserialize_mt(stream, table))
                    return false;
                
                out = CreateTableRecord(lsn, 0, txn_id, std::move(table));
                return true;
            }
            
        default:
            return false;
        }
    }
} // namespace wal
