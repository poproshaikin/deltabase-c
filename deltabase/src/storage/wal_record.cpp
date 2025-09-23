#include "include/wal/wal_record.hpp"
#include "include/shared.hpp"
#include <cstdint>
#include <cstring>

namespace storage {
    bytes_v
    InsertRecord::serialize() const {
        auto type = static_cast<std::underlying_type_t<WalRecordType>>(InsertRecord::type);
        auto type_ptr = reinterpret_cast<const uint64_t*>(&type);
        auto id = reinterpret_cast<const char*>(this->table_id.c_str());

        bytes_v v;
        v.reserve(sizeof(type) + strlen((id) + serialized_row.size()));

        v.insert(v.begin(), type_ptr, type_ptr + sizeof(type));
        v.insert(v.end(), id, id + strlen(id));
        v.insert(v.end(), serialized_row.begin(), serialized_row.end());
        return v;
    }
}