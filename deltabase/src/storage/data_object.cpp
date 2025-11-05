#include "include/objects/data_object.hpp"
#include "include/shared.hpp"
#include "include/value_type.hpp"
#include "../misc/include/utils.hpp"
#include <alloca.h>
#include <cstdint>
#include <stdexcept>

namespace storage
{
    bytes_v
    DataRow::serialize() const
    {
        auto estimate_size = [this]() -> uint64_t
        {
            uint64_t size = sizeof(row_id) + sizeof(flags) + sizeof(uint64_t);
            // row_id + flags + tokens count

            for (const auto& token : tokens)
                size += token.estimate_size();

            return size;
        };

        bytes_v buffer;
        buffer.reserve(estimate_size());
        MemoryStream stream(buffer);

        // Write row_id
        stream.write(&row_id, sizeof(row_id));

        // Write flags
        stream.write(&flags, sizeof(flags));

        // Write tokens count
        uint64_t tokens_count = tokens.size();
        stream.write(&tokens_count, sizeof(tokens_count));

        // Write each token
        for (const auto& token : tokens)
        {
            bytes_v serialized = token.serialize();
            stream.write(serialized.data(), serialized.size());
        }

        return buffer;
    }

    uint64_t
    get_type_size(ValueType type)
    {
        if (type == ValueType::STRING)
            return 0;

        switch (type)
        {
        case ValueType::INTEGER:
            return 4;
        case ValueType::REAL:
            return 8;
        case ValueType::BOOL:
            return 1;
        case ValueType::CHAR:
            return 1;
        default:
            throw std::runtime_error(
                "Unknown or unsupported ValueType: " + std::to_string(static_cast<int>(type))
            );
        }
    }

    uint64_t
    DataToken::estimate_size() const
    {
        uint64_t size = sizeof(ValueType); // size of the value type
        if (type == ValueType::STRING)
            size += sizeof(uint64_t); // size of the string length field

        size += bytes.size(); // actual data
        return size;
    }

    bytes_v
    DataToken::serialize() const
    {
        bytes_v buffer;
        buffer.reserve(estimate_size());

        MemoryStream stream(buffer);
        // Write type
        stream.write(&type, sizeof(type));

        // For strings, write length first
        if (type == ValueType::STRING)
        {
            uint64_t length = bytes.size();
            stream.write(&length, sizeof(length));
        }

        // Write bytes
        stream.write(bytes.data(), bytes.size());

        return buffer;
    }
}