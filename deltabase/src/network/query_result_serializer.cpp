#include "include/query_result_serializer.hpp"

#include "memory_stream.hpp"

namespace net
{
    using namespace misc;

    types::Bytes
    QueryResultSerializer::serialize(types::IExecutionResult& result) const
    {
        MemoryStream stream;

        const auto schema = result.output_schema();
        stream.write_u64(schema.size(), true);
        for (const auto& column : schema)
        {
            stream.write_string(column.name, true);
            stream.write_u64(static_cast<uint64_t>(column.type), true);
        }

        std::vector<types::DataRow> rows;
        types::DataRow row;
        while (result.next(row))
        {
            rows.push_back(row);
        }

        stream.write_u64(rows.size(), true);
        for (const auto& data_row : rows)
        {
            stream.write_u64(data_row.tokens.size(), true);
            for (const auto& token : data_row.tokens)
            {
                stream.write_u64(static_cast<uint64_t>(token.type), true);
                stream.write_bytes(token.bytes, true);
            }
        }

        return stream.to_vector();
    }

    // obsolete
    bool
    QueryResultSerializer::deserialize(
        const types::Bytes& bytes,
        types::OutputSchema& out_schema,
        std::vector<types::DataRow>& out_rows
    ) const
    {
        ReadOnlyMemoryStream stream(bytes);

        uint64_t column_count = 0;
        if (!stream.read_u64(column_count, true))
        {
            return false;
        }

        types::OutputSchema schema;
        schema.reserve(column_count);
        for (uint64_t i = 0; i < column_count; ++i)
        {
            std::string name;
            if (!stream.read_string(name, true))
            {
                return false;
            }

            uint64_t raw_type = 0;
            if (!stream.read_u64(raw_type, true))
            {
                return false;
            }

            schema.push_back(types::OutputColumn{
                .name = std::move(name),
                .type = static_cast<types::DataType>(raw_type),
            });
        }

        uint64_t row_count = 0;
        if (!stream.read_u64(row_count, true))
        {
            return false;
        }

        std::vector<types::DataRow> rows;
        rows.reserve(row_count);
        for (uint64_t i = 0; i < row_count; ++i)
        {
            uint64_t token_count = 0;
            if (!stream.read_u64(token_count, true))
            {
                return false;
            }

            types::DataRow row;
            row.tokens.reserve(token_count);
            for (uint64_t j = 0; j < token_count; ++j)
            {
                uint64_t raw_type = 0;

                types::Bytes token_bytes;
                if (!stream.read_bytes(token_bytes, true))
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
