#include "include/wal/wal_object.hpp"
#include "include/shared.hpp"
#include <cstdint>
#include <cstring>

namespace storage {
    bytes_v
    insert_record::serialize() const {
        auto lsn_ptr = ;
        auto type = static_cast<std::underlying_type_t<wal_record_type>>(insert_record::type);
        auto type_ptr = reinterpret_cast<const uint64_t*>(&type);
        auto id = reinterpret_cast<const char*>(this->table_id.c_str());

        bytes_v v;
        v.reserve(estimate_size());

        v.insert
        v.insert(v.begin(), type_ptr, type_ptr + sizeof(type));
        v.insert(v.end(), id, id + strlen(id));
        v.insert(v.end(), serialized_row.begin(), serialized_row.end());
        return v;
    }

    uint64_t
    insert_record::estimate_size() const {
        return sizeof(type) + table_id.length() + serialized_row.size();
    }

    uint64_t
    WalLogfile::size() const {
        uint64_t size = 0;
        for (const auto& record : records) {
            size += std::visit([](auto& record) { return record.estimate_size(); }, record);
        }
        return size;
    }
}