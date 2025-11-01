#include "include/pages/page.hpp"
#include "include/objects/data_object.hpp"
#include "include/objects/meta_object.hpp"
#include "../misc/include/utils.hpp"

#include <format>

namespace storage {
    DataPage::DataPage(std::string id, uint64_t size, RowId min_rid, RowId max_rid)
        : id_(id), size_(size), min_rid_(min_rid), max_rid_(max_rid)
    {
    }

    std::string
    DataPage::id() const {
        return id_;
    }

    std::string
    DataPage::table_id() const {
        return table_id_;
    }

    uint64_t
    DataPage::size() const {
        return size_;
    }
    
    RowId
    DataPage::min_rid() const {
        return min_rid_;
    }

    RowId
    DataPage::max_rid() const {
        return max_rid_;
    }

    void
    DataPage::mark_dirty() {
        is_dirty_ = true;
    }

    const std::vector<DataRow>&
    DataPage::rows() const {
        return rows_;
    }

    bool
    DataPage::has_row(RowId rid) const {
        for (const auto& row : rows_) {
            if (row.row_id == rid) {
                return true;
            }
        }

        return false;
    }

    DataRow&
    DataPage::get_row(RowId rid) {
        for (auto& row : rows_) {
            if (row.row_id == rid) {
                return row;
            }
        }

        throw std::runtime_error(std::format("Row with id {} was not found in DataPage {}", rid, id_));
    }

    bool
    DataPage::can_insert(const DataRow& row) const noexcept {
        return size_ + row.estimate_size() <= max_size;
    }

    RowId
    DataPage::insert_row(MetaTable& table, DataRow& row) {
        if (!can_insert(row)) {
            throw std::runtime_error(std::format("Page {} hasn't enough space for a new row", id_));
        }

        // Set table_id if not already set
        if (table_id_.empty()) {
            table_id_ = table.id;
        }

        row.row_id = ++table.last_rid;
        rows_.push_back(row);
        size_ += row.estimate_size();

        return row.row_id;
    }

    void
    DataPage::delete_row(RowId id) {
        if (!has_row(id)) {
            throw std::runtime_error("DataPage::delete_row");
        }

        get_row(id).flags |= DataRowFlags::OBSOLETE;
    }

    RowId
    DataPage::update_row(MetaTable& table, RowId old_row_id, DataRowUpdate& update) {
        if (!has_row(old_row_id)) {
            throw std::runtime_error("DataPage::update_row");
        }

        auto& old_row = get_row(old_row_id);
        old_row.flags |= DataRowFlags::OBSOLETE;

        auto new_row = old_row.update(update);
        insert_row(table, new_row);

        return new_row.row_id;
    }

    bytes_v
    DataPage::serialize() const {
        auto estimate_size = [this]() -> uint64_t {
            uint64_t size = sizeof(uint64_t) + id_.size(); // id length + id bytes
            size += sizeof(uint64_t) + table_id_.size(); // table_id length + table_id bytes
            size += sizeof(size_) + sizeof(min_rid_) + sizeof(max_rid_);
            
            for (const auto& row : rows_) {
                size += row.estimate_size();
            }
            
            return size;
        };

        bytes_v buffer;
        buffer.reserve(estimate_size());
        MemoryStream stream(buffer);

        // Write id with length prefix
        uint64_t id_length = id_.size();
        stream.write(&id_length, sizeof(id_length));
        stream.write(id_.data(), id_.size());

        // Write table_id with length prefix
        uint64_t table_id_length = table_id_.size();
        stream.write(&table_id_length, sizeof(table_id_length));
        stream.write(table_id_.data(), table_id_.size());

        // Write size, min_rid, max_rid
        stream.write(&size_, sizeof(size_));
        stream.write(&min_rid_, sizeof(min_rid_));
        stream.write(&max_rid_, sizeof(max_rid_));

        // Write each row
        for (const auto& row : rows_) {
            auto serialized_row = row.serialize();
            stream.write(serialized_row.data(), serialized_row.size());
        }

        return buffer;
    }
    
    bool
    DataPage::try_deserialize(const bytes_v& content, DataPage& out_result)
    {
        if (content.empty()) {
            return false;
        }

        ReadOnlyMemoryStream stream(content);

        // page id 
        uint64_t id_length;
        if (stream.read(&id_length, sizeof(id_length)) != sizeof(id_length))
            return false;

        if (id_length > 1024) // sanity check
            return false;

        std::string id_str(id_length, '\0');
        if (stream.read(id_str.data(), id_length) != id_length)
            return false;

        out_result.id_ = id_str;

        // table_id
        uint64_t table_id_length;
        if (stream.read(&table_id_length, sizeof(table_id_length)) != sizeof(table_id_length))
            return false;

        if (table_id_length > 1024) // sanity check
            return false;

        std::string table_id_str(table_id_length, '\0');
        if (stream.read(table_id_str.data(), table_id_length) != table_id_length)
            return false;

        out_result.table_id_ = table_id_str;

        // size
        if (stream.read(&out_result.size_, sizeof(uint64_t)) != sizeof(uint64_t))
            return false;

        // min rid
        if (stream.read(&out_result.min_rid_, sizeof(RowId)) != sizeof(RowId))
            return false;

        // max_rid
        if (stream.read(&out_result.max_rid_, sizeof(RowId)) != sizeof(RowId))
            return false;

        // rows
        out_result.rows_.clear();
        while (stream.remaining() > 0) {
            DataRow row;
            
            // row_id
            if (stream.read(&row.row_id, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;

            // flags
            if (stream.read(&row.flags, sizeof(DataRowFlags)) != sizeof(DataRowFlags))
                return false;

            // tokens count
            uint64_t tokens_count;
            if (stream.read(&tokens_count, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;

            if (tokens_count > max_tokens) // sanity check
                return false;

            // each token
            row.tokens.reserve(tokens_count);
            for (uint64_t i = 0; i < tokens_count; ++i) {
                DataToken token{bytes_v{}, ValueType::INTEGER};
                
                // Read ValueType
                if (stream.read(&token.type, sizeof(ValueType)) != sizeof(ValueType))
                    return false;

                // Read token bytes
                if (token.type == ValueType::STRING) {
                    // Read string length
                    uint64_t str_length;
                    if (stream.read(&str_length, sizeof(str_length)) != sizeof(str_length))
                        return false;

                    if (str_length > 1000000) // sanity check
                        return false;

                    token.bytes.resize(str_length);
                    if (stream.read(token.bytes.data(), str_length) != str_length)
                        return false;
                } else {
                    // Fixed-size type
                    uint64_t type_size = get_type_size(token.type);
                    token.bytes.resize(type_size);
                    if (stream.read(token.bytes.data(), type_size) != type_size)
                        return false;
                }

                row.tokens.push_back(std::move(token));
            }

            out_result.rows_.push_back(std::move(row));
        }

        out_result.is_dirty_ = false;
        return true;
    }
}