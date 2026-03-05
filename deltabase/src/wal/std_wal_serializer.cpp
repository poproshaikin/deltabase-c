//
// Created by poproshaikin on 04.03.26.
//

#include "include/std_wal_serializer.hpp"
namespace wal
{
    using namespace types;
    using namespace misc;

    MemoryStream
    StdWalSerializer::serialize(const BeginTransactionRecord& record) const
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

        auto serialized_row = binary_serializer_.serialize_dr(record.row);
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
        auto serialized_old_row = binary_serializer_.serialize_dr(record.old_row);
        stream.append(serialized_old_row, serialized_old_row.size());

        // Сериализуем new_row
        auto serialized_new_row = binary_serializer_.serialize_dr(record.new_row);
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

        auto serialized_row = binary_serializer_.serialize_dr(record.row);
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
    StdWalSerializer::serialize(const CommitTransactionRecord& record) const
    {
        MemoryStream stream;
        stream.write(&record.type, sizeof(record.type));
        stream.write(&record.lsn, sizeof(record.lsn));
        stream.write(&record.txn_id, sizeof(uuid_t));
        return stream;
    }

    MemoryStream
    StdWalSerializer::serialize(const WalRecord& record) const
    {
        switch (WalRecordType type = std::visit([](const auto& rec) { return rec.type; }, record))
        {
        case WalRecordType::INSERT:
            return serialize(std::get<InsertRecord>(record));
        case WalRecordType::UPDATE:
            return serialize(std::get<UpdateRecord>(record));
        case WalRecordType::DELETE:
            return serialize(std::get<DeleteRecord>(record));
        case WalRecordType::CREATE_TABLE:
            return serialize(std::get<CreateTableRecord>(record));
        case WalRecordType::CREATE_SCHEMA:
            return serialize(std::get<CreateSchemaRecord>(record));
        case WalRecordType::BEGIN_TRANSACTION:
            return serialize(std::get<BeginTransactionRecord>(record));
        case WalRecordType::COMMIT_TRANSACTION:
            return serialize(std::get<CommitTransactionRecord>(record));
        default:
            throw std::runtime_error("StdWalSerializer::serialize: Unknown WAL record type " + std::to_string(static_cast<int>(type)));
        }
    }
} // namespace wal
