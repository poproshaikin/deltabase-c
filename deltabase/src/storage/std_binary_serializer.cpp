//
// Created by poproshaikin on 30.11.25.
//

#include "std_binary_serializer.hpp"

namespace storage
{
    using namespace misc;
    using namespace types;

    void
    StdBinarySerializer::write_str(const std::string& str, MemoryStream& stream) const
    {
        uint64_t size = str.length();
        stream.write(&size, sizeof(uint64_t));
        stream.write(str.data(), size);
    }

    bool
    StdBinarySerializer::read_str(std::string& str, ReadOnlyMemoryStream& stream) const
    {
        uint64_t size = 0;
        if (stream.read(&size, sizeof(uint64_t)) != sizeof(size_t))
            return false;

        str.resize(size);
        if (stream.read(&str[0], size) != size)
            return false;

        return true;
    }

    bool
    StdBinarySerializer::has_dynamic_size(DataType data_type) const
    {
        switch (data_type)
        {
        case DataType::STRING:
            return true;
        default:
            return false;
        }
    }

    uint64_t
    StdBinarySerializer::get_data_type_size(DataType data_type) const
    {
        switch (data_type)
        {
        case DataType::INTEGER:
            return 4;
        case DataType::REAL:
            return 8;
        case DataType::BOOL:
            return 1;
        case DataType::CHAR:
            return 1;
        default:
            throw std::runtime_error(
                "StdBinarySerializer:get_data_type_size: failed to get size of type " +
                std::to_string(static_cast<int>(data_type))
            );
        }
    }

    MemoryStream
    StdBinarySerializer::serialize_mt(const MetaTable& table) const
    {
        MemoryStream stream;
        // Write UUID bytes directly (libuuid already stores in network byte order)
        stream.write(table.id.raw(), sizeof(uuid_t));
        stream.write(table.schema_id.raw(), sizeof(uuid_t));
        write_str(table.name, stream);

        uint64_t column_count = table.columns.size();
        stream.write(&column_count, sizeof(uint64_t));

        for (const auto& column : table.columns)
        {
            auto serialized_col = serialize_mc(column);
            stream.write(serialized_col.data(), serialized_col.size());
        }

        stream.write(&table.last_rid, sizeof(table.last_rid));
        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_ms(const MetaSchema& schema) const
    {
        MemoryStream stream;
        stream.write(schema.id.raw(), sizeof(uuid_t));
        write_str(schema.name, stream);
        write_str(schema.db_name, stream);
        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_mc(const MetaColumn& column) const
    {
        MemoryStream stream;
        stream.write(column.id.raw(), sizeof(uuid_t));
        stream.write(column.table_id.raw(), sizeof(uuid_t));
        write_str(column.name, stream);
        stream.write(&column.type, sizeof(column.type));
        stream.write(&column.flags, sizeof(column.flags));
        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_dph(const DataPageHeader& header) const
    {
        MemoryStream stream;
        stream.write(header.id.raw(), sizeof(uuid_t));
        stream.write(header.table_id.raw(), sizeof(uuid_t));
        stream.write(&header.min_rid, sizeof(header.min_rid));
        stream.write(&header.max_rid, sizeof(header.max_rid));
        stream.write(&header.rows_count, sizeof(header.rows_count));

        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_dt(const DataToken& token) const
    {
        MemoryStream stream;
        stream.write(&token.type, sizeof(token.type));
        uint64_t size = token.bytes.size();
        stream.write(&size, sizeof(uint64_t));
        stream.write(token.bytes.data(), size);

        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_dp(const DataPage& page) const
    {
        MemoryStream stream;

        // Create header copy with correct rows_count
        DataPageHeader header = page.header;
        header.rows_count = page.rows.size();
        
        auto serialized_header = serialize_dph(header);
        stream.append(serialized_header, serialized_header.size());

        for (size_t i = 0; i < page.rows.size(); ++i)
        {
            auto serialized_row = serialize_dr(page.rows[i]);
            stream.append(serialized_row, serialized_row.size());
        }

        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_dr(const DataRow& row) const
    {
        MemoryStream stream;
        stream.write(&row.id, sizeof(row.id));
        stream.write(&row.flags, sizeof(row.flags));

        uint64_t tokens_count = row.tokens.size();
        stream.write(&tokens_count, sizeof(uint64_t));

        for (size_t i = 0; i < row.tokens.size(); ++i)
        {
            MemoryStream serialized_token = serialize_dt(row.tokens[i]);
            stream.append(serialized_token, serialized_token.size());
        }

        stream.seek(0);
        return stream;
    }

    MemoryStream
    StdBinarySerializer::serialize_cfg(const Config& db) const
    {
        MemoryStream stream;
        write_str(db.db_name.value_or(""), stream);
        write_str(db.default_schema, stream);
        write_str(db.db_path.string(), stream);
        stream.write(&db.io_type, sizeof(db.io_type));
        stream.write(&db.planner_type, sizeof(db.planner_type));
        stream.write(&db.serializer_type, sizeof(db.serializer_type));
        stream.seek(0);
        return stream;
    }

    bool
    StdBinarySerializer::deserialize_mt(ReadOnlyMemoryStream& stream, MetaTable& out) const
    {
        stream.read(out.id.raw(), sizeof(uuid_t));
        stream.read(out.schema_id.raw(), sizeof(uuid_t));

        if (!read_str(out.name, stream))
            return false;

        uint64_t column_count = 0;
        if (stream.read(&column_count, sizeof(uint64_t)) != sizeof(size_t))
            return false;

        out.columns.clear();
        out.columns.reserve(column_count);

        for (uint64_t i = 0; i < column_count; ++i)
        {
            MetaColumn column;
            if (!deserialize_mc(stream, column))
                return false;

            out.columns.push_back(std::move(column));
        }

        if (stream.read(&out.last_rid, sizeof(out.last_rid)) != sizeof(out.last_rid))
            return false;

        return true;
    }

    bool
    StdBinarySerializer::deserialize_ms(ReadOnlyMemoryStream& stream, MetaSchema& out) const
    {
        stream.read(out.id.raw(), sizeof(uuid_t));

        if (!read_str(out.name, stream))
            return false;

        if (!read_str(out.db_name, stream))
            return false;

        return true;
    }

    bool
    StdBinarySerializer::deserialize_mc(ReadOnlyMemoryStream& stream, MetaColumn& out) const
    {
        if (stream.read(out.id.raw(), sizeof(uuid_t)) != sizeof(uuid_t))
            return false;
        if (stream.read(out.table_id.raw(), sizeof(uuid_t)) != sizeof(uuid_t))
            return false;

        if (!read_str(out.name, stream))
            return false;

        if (stream.read(&out.type, sizeof(out.type)) != sizeof(out.type))
            return false;
        if (stream.read(&out.flags, sizeof(out.flags)) != sizeof(out.flags))
            return false;

        return true;
    }

    bool
    StdBinarySerializer::deserialize_dp(ReadOnlyMemoryStream& stream, DataPage& out) const
    {
        if (!deserialize_dph(stream, out.header))
            return false;

        // rows_count already read in header
        uint64_t rows_count = out.header.rows_count;

        out.rows.clear();
        out.rows.reserve(rows_count);

        for (size_t i = 0; i < rows_count; ++i)
        {
            DataRow row;
            if (!deserialize_dr(stream, row))
                return false;

            out.rows.push_back(std::move(row));
        }

        return true;
    }

    bool
    StdBinarySerializer::deserialize_dph(ReadOnlyMemoryStream& stream, DataPageHeader& out) const
    {
        if (stream.read(out.id.raw(), sizeof(uuid_t)) != sizeof(uuid_t))
            return false;

        if (stream.read(out.table_id.raw(), sizeof(uuid_t)) != sizeof(uuid_t))
            return false;

        if (stream.read(&out.min_rid, sizeof(out.min_rid)) != sizeof(out.min_rid))
            return false;

        if (stream.read(&out.max_rid, sizeof(out.max_rid)) != sizeof(out.max_rid))
            return false;

        if (stream.read(&out.rows_count, sizeof(out.rows_count)) != sizeof(out.rows_count))
            return false;

        return true;
    }

    bool
    StdBinarySerializer::deserialize_cfg(ReadOnlyMemoryStream& stream, Config& out) const
    {
        std::string db_name;
        if (!read_str(db_name, stream))
            return false;
        out.db_name = db_name;

        if (!read_str(out.default_schema, stream))
            return false;

        std::string db_path_str;
        if (!read_str(db_path_str, stream))
            return false;

        out.db_path = fs::path(db_path_str);

        if (stream.read(&out.io_type, sizeof(out.io_type)) != sizeof(out.io_type))
            return false;

        if (stream.read(&out.planner_type, sizeof(out.planner_type)) != sizeof(out.planner_type))
            return false;

        if (stream.read(&out.serializer_type, sizeof(out.serializer_type)) !=
            sizeof(out.serializer_type))
            return false;

        return true;
    }

    bool
    StdBinarySerializer::deserialize_dr(ReadOnlyMemoryStream& stream, DataRow& out) const
    {
        if (stream.read(&out.id, sizeof(out.id)) != sizeof(out.id))
            return false;

        if (stream.read(&out.flags, sizeof(out.flags)) != sizeof(out.flags))
            return false;
        
        uint64_t tokens_count = 0;
        if (stream.read(&tokens_count, sizeof(uint64_t)) != sizeof(uint64_t))
            return false;

        out.tokens.clear();
        out.tokens.reserve(tokens_count);

        for (uint64_t i = 0; i < tokens_count; ++i)
        {
            DataToken token;
            if (!deserialize_dt(stream, token))
            {
                return false;
            }
            out.tokens.push_back(std::move(token));
        }

        return true;
    }

    bool
    StdBinarySerializer::deserialize_dt(ReadOnlyMemoryStream& stream, DataToken& out) const
    {
        if (stream.read(&out.type, sizeof(out.type)) != sizeof(out.type))
        {
            std::cout << "    [deserialize_dt] ERROR: Failed to read type" << std::endl;
            return false;
        }

        uint64_t size = 0;
        if (stream.read(&size, sizeof(uint64_t)) != sizeof(uint64_t))
        {
            std::cout << "    [deserialize_dt] ERROR: Failed to read size" << std::endl;
            return false;
        }

        out.bytes.resize(size);
        if (stream.read(out.bytes.data(), size) != size)
        {
            std::cout << "    [deserialize_dt] ERROR: failed to read " << size << " bytes" << std::endl;
            return false;
        }

        return true;
    }

    uint64_t
    StdBinarySerializer::estimate_size(const DataRow& row) const
    {
        uint64_t size = sizeof(row.id) + sizeof(row.flags) + sizeof(uint64_t);
        for (const auto& token : row.tokens)
            size += estimate_size(token);
        return size;
    }

    uint64_t
    StdBinarySerializer::estimate_size(const DataToken& token) const
    {
        uint64_t size = sizeof(token.type);

        if (has_dynamic_size(token.type))
            size += sizeof(uint64_t); // bytes count
        size += token.bytes.size();

        return size;
    }
} // namespace storage