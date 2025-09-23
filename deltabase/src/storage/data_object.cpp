#pragma once

#include "include/objects/data_object.hpp"
#include "include/shared.hpp"
#include "include/value_type.hpp"
#include <alloca.h>
#include <cstdint>
#include <stdexcept>

namespace storage {
    bytes_v
    DataRow::serialize() const {
        auto estimate_rows_size = [this]() -> uint64_t {
            uint64_t size = 0;
            for (const auto& token : tokens) {
                size += token.estimate_size();
            }
            return size;
        };

        auto serialize_rows = [this, estimate_rows_size]() -> bytes_v {
            bytes_v v;
            v.reserve(estimate_rows_size());
            for (const auto& token : tokens) {
                bytes_v serialized = token.serialize();
                v.insert(v.end(), serialized.begin(), serialized.end());
            }
            return v;
        };

        bytes_v v;
        v.reserve(sizeof(row_id) + sizeof(flags) + estimate_rows_size());

        auto rid_ptr = &row_id;
        auto drf_ptr = reinterpret_cast<const uint64_t*>(&flags);
        bytes_v serialized_rows = serialize_rows();

        v.insert(v.end(), rid_ptr, rid_ptr + sizeof(row_id));
        v.insert(v.end(), drf_ptr, drf_ptr + sizeof(flags));
        v.insert(v.end(), serialized_rows.begin(), serialized_rows.end());

        return v;
    }

    uint64_t
    get_type_size(ValueType type) {
        if (type == ValueType::STRING) {
            return 0;
        }

        switch (type) {
            case ValueType::INTEGER: return 4;
            case ValueType::REAL:    return 8;
            case ValueType::BOOL:    return 1;
            case ValueType::CHAR:    return 1;

            default: throw std::runtime_error("Unknown or unsupported ValueType: " + std::to_string(static_cast<int>(type)));
        }
    }

    uint64_t
    DataToken::estimate_size() const {
        uint64_t size = 0; 
        size += sizeof(ValueType); // size of the value type prefix
        size += bytes.size();
        return size;
    }

    bytes_v
    DataToken::serialize() const {
        bytes_v v;
        v.reserve(estimate_size());

        auto type_ptr = reinterpret_cast<const uint64_t*>(&type);
        v.insert(v.end(), type_ptr, type_ptr + sizeof(type));
        v.insert(v.end(), bytes.begin(), bytes.end());

        return v;
    }
}