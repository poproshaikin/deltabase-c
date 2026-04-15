#include "include/query_result_serializer.hpp"

#include "memory_stream.hpp"

namespace net
{
    using namespace misc;

    namespace
    {
        void
        write_u64(MemoryStream& stream, uint64_t value)
        {
            stream.write(&value, sizeof(value));
        }

        bool
        read_u64(ReadOnlyMemoryStream& stream, uint64_t& out)
        {
            return stream.read(&out, sizeof(out)) == sizeof(out);
        }

        void
        write_string(MemoryStream& stream, const std::string& value)
        {
            const auto size = static_cast<uint64_t>(value.size());
            write_u64(stream, size);
            if (!value.empty())
            {
                stream.write(value.data(), value.size());
            }
        }

        bool
        read_string(ReadOnlyMemoryStream& stream, std::string& out)
        {
            uint64_t size = 0;
            if (!read_u64(stream, size))
            {
                return false;
            }

            out.resize(size);
            if (size == 0)
            {
                return true;
            }

            return stream.read(out.data(), size) == size;
        }

        void
        write_bytes(MemoryStream& stream, const types::Bytes& bytes)
        {
            const auto size = static_cast<uint64_t>(bytes.size());
            write_u64(stream, size);
            if (!bytes.empty())
            {
                stream.write(bytes.data(), bytes.size());
            }
        }

        bool
        read_bytes(ReadOnlyMemoryStream& stream, types::Bytes& out)
        {
            uint64_t size = 0;
            if (!read_u64(stream, size))
            {
                return false;
            }

            out.resize(size);
            if (size == 0)
            {
                return true;
            }

            return stream.read(out.data(), size) == size;
        }
    } // namespace

    types::Bytes
    QueryResultSerializer::serialize(types::IExecutionResult& result) const
    {
        MemoryStream stream;

        const auto schema = result.output_schema();
        write_u64(stream, static_cast<uint64_t>(schema.size()));
        for (const auto& column : schema)
        {
            write_string(stream, column.name);
            write_u64(stream, static_cast<uint64_t>(column.type));
        }

        std::vector<types::DataRow> rows;
        types::DataRow row;
        while (result.next(row))
        {
            rows.push_back(row);
        }

        write_u64(stream, static_cast<uint64_t>(rows.size()));
        for (const auto& data_row : rows)
        {
            write_u64(stream, static_cast<uint64_t>(data_row.tokens.size()));
            for (const auto& token : data_row.tokens)
            {
                write_u64(stream, static_cast<uint64_t>(token.type));
                write_bytes(stream, token.bytes);
            }
        }

        return stream.to_vector();
    }

    bool
    QueryResultSerializer::deserialize(
        const types::Bytes& bytes,
        types::OutputSchema& out_schema,
        std::vector<types::DataRow>& out_rows
    ) const
    {
        ReadOnlyMemoryStream stream(bytes);

        uint64_t column_count = 0;
        if (!read_u64(stream, column_count))
        {
            return false;
        }

        types::OutputSchema schema;
        schema.reserve(column_count);
        for (uint64_t i = 0; i < column_count; ++i)
        {
            std::string name;
            if (!read_string(stream, name))
            {
                return false;
            }

            uint64_t raw_type = 0;
            if (!read_u64(stream, raw_type))
            {
                return false;
            }

            schema.push_back(types::OutputColumn{
                .name = std::move(name),
                .type = static_cast<types::DataType>(raw_type),
            });
        }

        uint64_t row_count = 0;
        if (!read_u64(stream, row_count))
        {
            return false;
        }

        std::vector<types::DataRow> rows;
        rows.reserve(row_count);
        for (uint64_t i = 0; i < row_count; ++i)
        {
            uint64_t token_count = 0;
            if (!read_u64(stream, token_count))
            {
                return false;
            }

            types::DataRow row;
            row.tokens.reserve(token_count);
            for (uint64_t j = 0; j < token_count; ++j)
            {
                uint64_t raw_type = 0;
                if (!read_u64(stream, raw_type))
                {
                    return false;
                }

                types::Bytes token_bytes;
                if (!read_bytes(stream, token_bytes))
                {
                    return false;
                }

                row.tokens.emplace_back(token_bytes, static_cast<types::DataType>(raw_type));
            }

            rows.push_back(std::move(row));
        }

        out_schema = std::move(schema);
        out_rows = std::move(rows);
        return true;
    }
} // namespace net
