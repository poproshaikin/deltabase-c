#include "include/wal/wal_object.hpp"
#include "include/shared.hpp"
#include "../misc/include/utils.hpp"
#include <cstdint>
#include <stdbool.h>

namespace storage
{
    bytes_v
    InsertRecord::serialize() const
    {
        bytes_v v;
        v.reserve(estimate_size());
        MemoryStream stream(v);
        
        // Write LSN
        stream.write(&lsn, sizeof(lsn));
        
        // Write type
        auto type_value = static_cast<std::underlying_type_t<WalRecordType>>(InsertRecord::type);
        stream.write(&type_value, sizeof(type_value));
        
        // Write table_id with length prefix
        uint64_t table_id_length = table_id.size();
        stream.write(&table_id_length, sizeof(table_id_length));
        stream.write(table_id.data(), table_id.size());
        
        // Write serialized_row with length prefix
        uint64_t row_length = serialized_row.size();
        stream.write(&row_length, sizeof(row_length));
        stream.write(serialized_row.data(), serialized_row.size());

        return v;
    }

    uint64_t
    InsertRecord::estimate_size() const
    {
        return sizeof(lsn)                    // LSN
             + sizeof(WalRecordType)          // type
             + sizeof(uint64_t)               // table_id length
             + table_id.size()                // table_id data
             + sizeof(uint64_t)               // serialized_row length
             + serialized_row.size();         // serialized_row data
    }

    bool
    InsertRecord::try_deserialize(const bytes_v& content, InsertRecord& out)
    {
        if (content.empty()) {
            return false;
        }

        ReadOnlyMemoryStream stream(content);

        // lsn
        if (stream.read(&out.lsn, sizeof(out.lsn)) != sizeof(out.lsn))
            return false;

        // type
        WalRecordType type_value;
        if (stream.read(&type_value, sizeof(type_value)) != sizeof(type_value))
            return false;

        if (type_value != InsertRecord::type)
            return false;

        // table_id
        uint64_t table_id_length;
        if (stream.read(&table_id_length, sizeof(table_id_length)) != sizeof(table_id_length))
            return false;

        if (table_id_length > 1024) // sanity check
            return false;

        std::string table_id(table_id_length, '\0');
        if (stream.read(table_id.data(), table_id_length) != table_id_length)
            return false;

        out.table_id = table_id;

        // serialized_row
        uint64_t row_length;
        if (stream.read(&row_length, sizeof(row_length)) != sizeof(row_length))
            return false;

        if (row_length > 10 * 1024 * 1024) // sanity check: 10 MB max
            return false;

        bytes_v row_data(row_length);
        if (stream.read(row_data.data(), row_length) != row_length)
            return false;

        out.serialized_row = std::move(row_data);

        return true;
    }

    // ===== CreateSchemaRecord =====

    bytes_v
    CreateSchemaRecord::serialize() const
    {
        bytes_v v;
        v.reserve(estimate_size());
        MemoryStream stream(v);
        
        // lsn
        stream.write(&lsn, sizeof(lsn));
        
        // type
        auto type_value = static_cast<std::underlying_type_t<WalRecordType>>(CreateSchemaRecord::type);
        stream.write(&type_value, sizeof(type_value));
        
        // schema_id with length prefix
        uint64_t schema_id_length = schema_id.size();
        stream.write(&schema_id_length, sizeof(schema_id_length));
        stream.write(schema_id.data(), schema_id.size());
        
        // serialized_schema with length prefix
        uint64_t schema_length = serialized_schema.size();
        stream.write(&schema_length, sizeof(schema_length));
        stream.write(serialized_schema.data(), serialized_schema.size());

        return v;
    }

    uint64_t
    CreateSchemaRecord::estimate_size() const
    {
        return sizeof(lsn)
             + sizeof(WalRecordType)
             + sizeof(uint64_t)
             + schema_id.size()
             + sizeof(uint64_t)
             + serialized_schema.size();
    }

    bool
    CreateSchemaRecord::try_deserialize(const bytes_v& content, CreateSchemaRecord& out)
    {
        if (content.empty()) {
            return false;
        }

        ReadOnlyMemoryStream stream(content);

        // LSN
        if (stream.read(&out.lsn, sizeof(out.lsn)) != sizeof(out.lsn))
            return false;

        // type
        WalRecordType type_value;
        if (stream.read(&type_value, sizeof(type_value)) != sizeof(type_value))
            return false;

        if (type_value != CreateSchemaRecord::type)
            return false;

        // schema_id
        uint64_t schema_id_length;
        if (stream.read(&schema_id_length, sizeof(schema_id_length)) != sizeof(schema_id_length))
            return false;

        if (schema_id_length > 1024)
            return false;

        std::string schema_id(schema_id_length, '\0');
        if (stream.read(schema_id.data(), schema_id_length) != schema_id_length)
            return false;

        out.schema_id = schema_id;

        // serialized_schema
        uint64_t schema_length;
        if (stream.read(&schema_length, sizeof(schema_length)) != sizeof(schema_length))
            return false;

        if (schema_length > 1024 * 1024) // 1 MB max
            return false;

        bytes_v schema_data(schema_length);
        if (stream.read(schema_data.data(), schema_length) != schema_length)
            return false;

        out.serialized_schema = std::move(schema_data);

        return true;
    }

    // ===== DropSchemaRecord =====

    bytes_v
    DropSchemaRecord::serialize() const
    {
        bytes_v v;
        v.reserve(estimate_size());
        MemoryStream stream(v);
        
        // LSN
        stream.write(&lsn, sizeof(lsn));
        
        // type
        auto type_value = static_cast<std::underlying_type_t<WalRecordType>>(DropSchemaRecord::type);
        stream.write(&type_value, sizeof(type_value));
        
        // schema_id with length prefix
        uint64_t schema_id_length = schema_id.size();
        stream.write(&schema_id_length, sizeof(schema_id_length));
        stream.write(schema_id.data(), schema_id.size());

        return v;
    }

    uint64_t
    DropSchemaRecord::estimate_size() const
    {
        return sizeof(lsn)
             + sizeof(WalRecordType)
             + sizeof(uint64_t)
             + schema_id.size();
    }

    bool
    DropSchemaRecord::try_deserialize(const bytes_v& content, DropSchemaRecord& out)
    {
        if (content.empty()) {
            return false;
        }

        ReadOnlyMemoryStream stream(content);

        // LSN
        if (stream.read(&out.lsn, sizeof(out.lsn)) != sizeof(out.lsn))
            return false;

        // type
        WalRecordType type_value;
        if (stream.read(&type_value, sizeof(type_value)) != sizeof(type_value))
            return false;

        if (type_value != DropSchemaRecord::type)
            return false;

        // schema_id
        uint64_t schema_id_length;
        if (stream.read(&schema_id_length, sizeof(schema_id_length)) != sizeof(schema_id_length))
            return false;

        if (schema_id_length > 1024)
            return false;

        std::string schema_id(schema_id_length, '\0');
        if (stream.read(schema_id.data(), schema_id_length) != schema_id_length)
            return false;

        out.schema_id = schema_id;

        return true;
    }

    // ===== CreateTableRecord =====

    bytes_v
    CreateTableRecord::serialize() const
    {
        bytes_v v;
        v.reserve(estimate_size());
        MemoryStream stream(v);
        
        // LSN
        stream.write(&lsn, sizeof(lsn));
        
        // type
        auto type_value = static_cast<std::underlying_type_t<WalRecordType>>(CreateTableRecord::type);
        stream.write(&type_value, sizeof(type_value));
        
        // table_id with length prefix
        uint64_t table_id_length = table_id.size();
        stream.write(&table_id_length, sizeof(table_id_length));
        stream.write(table_id.data(), table_id.size());
        
        // serialized_table with length prefix
        uint64_t table_length = serialized_table.size();
        stream.write(&table_length, sizeof(table_length));
        stream.write(serialized_table.data(), serialized_table.size());

        return v;
    }

    uint64_t
    CreateTableRecord::estimate_size() const
    {
        return sizeof(lsn)
             + sizeof(WalRecordType)
             + sizeof(uint64_t)
             + table_id.size()
             + sizeof(uint64_t)
             + serialized_table.size();
    }

    bool
    CreateTableRecord::try_deserialize(const bytes_v& content, CreateTableRecord& out)
    {
        if (content.empty()) 
            return false;

        ReadOnlyMemoryStream stream(content);

        // LSN
        if (stream.read(&out.lsn, sizeof(out.lsn)) != sizeof(out.lsn))
            return false;

        // type
        WalRecordType type_value;
        if (stream.read(&type_value, sizeof(type_value)) != sizeof(type_value))
            return false;

        if (type_value != CreateTableRecord::type)
            return false;

        // table_id
        uint64_t table_id_length;
        if (stream.read(&table_id_length, sizeof(table_id_length)) != sizeof(table_id_length))
            return false;

        if (table_id_length > 1024)
            return false;

        std::string table_id(table_id_length, '\0');
        if (stream.read(table_id.data(), table_id_length) != table_id_length)
            return false;

        out.table_id = table_id;

        // serialized_table
        uint64_t table_length;
        if (stream.read(&table_length, sizeof(table_length)) != sizeof(table_length))
            return false;

        if (table_length > 10 * 1024 * 1024) // 10 MB max
            return false;

        bytes_v table_data(table_length);
        if (stream.read(table_data.data(), table_length) != table_length)
            return false;

        out.serialized_table = std::move(table_data);

        return true;
    }

    // ===== WalLogfile =====

    WalLogfile::WalLogfile(std::filesystem::path path) : path(path)
    {
    }

    uint64_t
    WalLogfile::first_lsn() const
    {
        if (records.empty()) {
            return 0;
        }

        return std::visit([](const auto& record) { return record.lsn; }, records.front());
    }

    uint64_t
    WalLogfile::last_lsn() const
    {
        if (records.empty()) {
            return 0;
        }

        return std::visit([](const auto& record) { return record.lsn; }, records.back());
    }

    uint64_t
    WalLogfile::size() const
    {
        uint64_t size = 0;
        for (const auto& record : records)
        {
            size += std::visit([](const auto& record) { return record.estimate_size(); }, record);
        }

        return size;
    }

    bytes_v
    WalLogfile::serialize()
    {
        bytes_v v;
        
        uint64_t total_size = sizeof(uint64_t);
        for (const auto& record : records)
            total_size += std::visit([](const auto& r) { return r.estimate_size(); }, record);

        v.reserve(total_size);
        MemoryStream stream(v);
        
        uint64_t record_count = records.size();
        stream.write(&record_count, sizeof(record_count));

        for (const auto& record : records)
        {
            bytes_v serialized = std::visit([](const auto& r) { return r.serialize(); }, record);
            stream.write(serialized.data(), serialized.size());
        }

        return v;
    }

    bool
    WalLogfile::try_deserialize(bytes_v content, WalLogfile& out)
    {
        if (content.empty())
            return false;

        ReadOnlyMemoryStream stream(content);

        uint64_t record_count;
        if (stream.read(&record_count, sizeof(record_count)) != sizeof(record_count))
            return false;

        if (record_count > 1000000) 
            return false;

        out.records.clear();
        out.records.reserve(record_count);

        for (uint64_t i = 0; i < record_count; ++i)
        {
            WalRecordType type;
            if (stream.read(&type, sizeof(type)) != sizeof(type))
                return false;

            stream.seek(stream.tell() - sizeof(type));

            bytes_v record_data(content.begin() + stream.tell(), content.end());

            switch (type)
            {
            case WalRecordType::INSERT:
            {
                InsertRecord record;
                if (!InsertRecord::try_deserialize(record_data, record))
                    return false;

                // Advance stream position
                stream.seek(stream.tell() + record.estimate_size());
                out.records.push_back(std::move(record));
                break;
            }
            case WalRecordType::CREATE_SCHEMA:
            {
                CreateSchemaRecord record;
                if (!CreateSchemaRecord::try_deserialize(record_data, record))
                    return false;

                stream.seek(stream.tell() + record.estimate_size());
                out.records.push_back(std::move(record));
                break;
            }
            case WalRecordType::DROP_SCHEMA:
            {
                DropSchemaRecord record;
                if (!DropSchemaRecord::try_deserialize(record_data, record))
                    return false;

                stream.seek(stream.tell() + record.estimate_size());
                out.records.push_back(std::move(record));
                break;
            }
            case WalRecordType::CREATE_TABLE:
            {
                CreateTableRecord record;
                if (!CreateTableRecord::try_deserialize(record_data, record))
                    return false;

                stream.seek(stream.tell() + record.estimate_size());
                out.records.push_back(std::move(record));
                break;
            }
            default:
                return false; // Unknown record type
            }
        }

        return true;
    }
} // namespace storage