//
// Created by poproshaikin on 30.11.25.
//

#include "std_binary_serializer.hpp"
#include "../misc/include/serialization/schema_fields.hpp"

#include <stdexcept>

namespace
{
    using namespace misc::serialization;
    using namespace types;

    template <typename TObject, typename... TFields>
    Schema<TObject>
    build_schema(TFields&&... fields)
    {
        Schema<TObject> schema;
        (schema.fields.push_back(std::forward<TFields>(fields)), ...);
        return schema;
    }

    const Schema<DataToken>&
    data_token_schema();

    const Schema<DataRow>&
    data_row_schema();

    const Schema<RowPtr>&
    row_ptr_schema();

    const Schema<LeafIndexNode>&
    leaf_node_schema();

    const Schema<InternalIndexNode>&
    internal_node_schema();

    const Schema<IndexPage>&
    index_page_schema();

    const Schema<MetaSchema>&
    meta_schema_schema()
    {
        static const auto SCHEMA = build_schema<MetaSchema>(
            make_uuid_field(&MetaSchema::id),
            make_string_field(&MetaSchema::name),
            make_string_field(&MetaSchema::db_name)
        );

        return SCHEMA;
    }

    const Schema<MetaColumn>&
    meta_column_schema()
    {
        static const auto SCHEMA = build_schema<MetaColumn>(
            make_uuid_field(&MetaColumn::id),
            make_uuid_field(&MetaColumn::table_id),
            make_string_field(&MetaColumn::name),
            make_pod_field(&MetaColumn::type),
            make_pod_field(&MetaColumn::flags)
        );

        return SCHEMA;
    }

    const Schema<MetaIndex>&
    meta_index_schema()
    {
        static const auto SCHEMA = build_schema<MetaIndex>(
            make_uuid_field(&MetaIndex::id),
            make_uuid_field(&MetaIndex::table_id),
            make_uuid_field(&MetaIndex::column_id),
            make_uuid_field(&MetaIndex::root_page_id),
            make_string_field(&MetaIndex::name),
            make_pod_field(&MetaIndex::is_unique),
            make_pod_field(&MetaIndex::key_type)
        );

        return SCHEMA;
    }

    const Schema<DataToken>&
    data_token_schema()
    {
        static const auto SCHEMA = build_schema<DataToken>(
            make_pod_field(&DataToken::type),
            make_conditional_field<DataToken>(
                [](const DataToken& token)
                {
                    return token.type != DataType::_NULL;
                },
                make_sized_vector_field(&DataToken::bytes),
                [](DataToken& token)
                {
                    token.bytes.clear();
                }
            )
        );

        return SCHEMA;
    }

    const Schema<DataRow>&
    data_row_schema()
    {
        static const auto SCHEMA = build_schema<DataRow>(
            make_pod_field(&DataRow::id),
            make_pod_field(&DataRow::flags),
            make_vector_object_field(&DataRow::tokens, &data_token_schema())
        );

        return SCHEMA;
    }

    const Schema<MetaTable>&
    meta_table_schema()
    {
        static const auto SCHEMA = build_schema<MetaTable>(
            make_uuid_field(&MetaTable::id),
            make_uuid_field(&MetaTable::schema_id),
            make_string_field(&MetaTable::name),
            make_pod_field(&MetaTable::total_rows),
            make_pod_field(&MetaTable::live_rows),
            make_vector_object_field(&MetaTable::columns, &meta_column_schema()),
            make_pod_field(&MetaTable::last_rid),
            make_vector_object_field(&MetaTable::indexes, &meta_index_schema())
        );

        return SCHEMA;
    }

    const Schema<DataPage>&
    data_page_schema()
    {
        static const auto SCHEMA = build_schema<DataPage>(
            make_uuid_field(&DataPage::id),
            make_uuid_field(&DataPage::table_id),
            make_pod_field(&DataPage::min_rid),
            make_pod_field(&DataPage::max_rid),
            make_vector_count_field(&DataPage::rows_count, &DataPage::rows),
            make_pod_field(&DataPage::last_lsn),
            make_vector_object_payload_field(&DataPage::rows, &DataPage::rows_count, &data_row_schema())
        );

        return SCHEMA;
    }

    const Schema<IndexPage>&
    index_page_schema();

    const Schema<RowPtr>&
    row_ptr_schema()
    {
        static const auto SCHEMA = build_schema<RowPtr>(
            make_uuid_field(&RowPtr::first),
            make_pod_field(&RowPtr::second)
        );

        return SCHEMA;
    }

    const Schema<LeafIndexNode>&
    leaf_node_schema()
    {
        static const auto SCHEMA = build_schema<LeafIndexNode>(
            make_vector_object_field(&LeafIndexNode::keys, &data_token_schema()),
            make_vector_object_field(&LeafIndexNode::rows, &row_ptr_schema()),
            make_pod_field(&LeafIndexNode::next_leaf)
        );

        return SCHEMA;
    }

    const Schema<InternalIndexNode>&
    internal_node_schema()
    {
        static const auto SCHEMA = build_schema<InternalIndexNode>(
            make_vector_object_field(&InternalIndexNode::keys, &data_token_schema()),
            make_vector_pod_field(&InternalIndexNode::children)
        );

        return SCHEMA;
    }

    const Schema<IndexFile>&
    index_file_schema()
    {
        static const auto SCHEMA = build_schema<IndexFile>(
            make_uuid_field(&IndexFile::index_id),
            make_pod_field(&IndexFile::root_page),
            make_pod_field(&IndexFile::last_page),
            make_vector_object_field(&IndexFile::pages, &index_page_schema())
        );

        return SCHEMA;
    }

    const Schema<Config>&
    config_schema()
    {
        static const auto SCHEMA = build_schema<Config>(
            make_optional_string_field(&Config::db_name),
            make_string_field(&Config::default_schema),
            make_path_field(&Config::db_path),
            make_pod_field(&Config::io_type),
            make_pod_field(&Config::planner_type),
            make_pod_field(&Config::serializer_type),
            make_pod_field(&Config::last_checkpoint_lsn)
        );

        return SCHEMA;
    }

    const Schema<IndexPage>&
    index_page_schema()
    {
        static const auto SCHEMA = build_schema<IndexPage>(
            make_pod_field(&IndexPage::id),
            make_pod_field(&IndexPage::parent),
            make_uuid_field(&IndexPage::index_id),
            make_pod_field(&IndexPage::is_leaf),
            make_bool_variant_field(
                &IndexPage::is_leaf,
                &IndexPage::data,
                &leaf_node_schema(),
                &internal_node_schema()
            )
        );

        return SCHEMA;
    }

    template <typename TObject>
    misc::MemoryStream
    serialize_object(const TObject& object, const Schema<TObject>& schema, const char* error)
    {
        misc::MemoryStream stream;
        WriteCursor cursor(stream);
        if (!write_by_schema(object, schema, cursor))
            throw std::runtime_error(error);

        stream.seek(0);
        return stream;
    }

    template <typename TObject>
    bool
    deserialize_object(misc::ReadOnlyMemoryStream& stream, const Schema<TObject>& schema, TObject& out)
    {
        ReadCursor cursor(stream);
        return read_by_schema(cursor, schema, out);
    }
} // namespace

namespace storage
{
    using namespace misc;
    using namespace types;
    using namespace misc::serialization;

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

    MemoryStream
    StdBinarySerializer::serialize_mt(const MetaTable& table) const
    {
        return serialize_object(table, meta_table_schema(), "StdBinarySerializer::serialize_mt");
    }

    MemoryStream
    StdBinarySerializer::serialize_ms(const MetaSchema& meta_schema) const
    {
        return serialize_object(
            meta_schema,
            meta_schema_schema(),
            "StdBinarySerializer::serialize_ms: schema write failed"
        );
    }

    MemoryStream
    StdBinarySerializer::serialize_mc(const MetaColumn& column) const
    {
        return serialize_object(column, meta_column_schema(), "StdBinarySerializer::serialize_mc");
    }

    MemoryStream
    StdBinarySerializer::serialize_mi(const MetaIndex& index) const
    {
        return serialize_object(index, meta_index_schema(), "StdBinarySerializer::serialize_mi");
    }

    MemoryStream
    StdBinarySerializer::serialize_dt(const DataToken& token) const
    {
        return serialize_object(token, data_token_schema(), "StdBinarySerializer::serialize_dt");
    }

    MemoryStream
    StdBinarySerializer::serialize_dp(const DataPage& page) const
    {
        return serialize_object(page, data_page_schema(), "StdBinarySerializer::serialize_dp");
    }

    MemoryStream
    StdBinarySerializer::serialize_if(const IndexFile& file) const
    {
        return serialize_object(file, index_file_schema(), "StdBinarySerializer::serialize_if");
    }

    MemoryStream
    StdBinarySerializer::serialize_ip(const IndexPage& page) const
    {
        return serialize_object(page, index_page_schema(), "StdBinarySerializer::serialize_ip");
    }

    MemoryStream
    StdBinarySerializer::serialize_dr(const DataRow& row) const
    {
        return serialize_object(row, data_row_schema(), "StdBinarySerializer::serialize_dr");
    }

    MemoryStream
    StdBinarySerializer::serialize_cfg(const Config& db) const
    {
        return serialize_object(db, config_schema(), "StdBinarySerializer::serialize_cfg");
    }

    bool
    StdBinarySerializer::deserialize_mt(ReadOnlyMemoryStream& stream, MetaTable& out) const
    {
        return deserialize_object(stream, meta_table_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_ms(ReadOnlyMemoryStream& stream, MetaSchema& out) const
    {
        return deserialize_object(stream, meta_schema_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_mc(ReadOnlyMemoryStream& stream, MetaColumn& out) const
    {
        return deserialize_object(stream, meta_column_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_mi(ReadOnlyMemoryStream& stream, MetaIndex& out) const
    {
        return deserialize_object(stream, meta_index_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_dp(ReadOnlyMemoryStream& stream, DataPage& out) const
    {
        return deserialize_object(stream, data_page_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_if(ReadOnlyMemoryStream& content, IndexFile& out) const
    {
        return deserialize_object(content, index_file_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_ip(ReadOnlyMemoryStream& content, IndexPage& out) const
    {
        return deserialize_object(content, index_page_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_cfg(ReadOnlyMemoryStream& stream, Config& out) const
    {
        return deserialize_object(stream, config_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_dr(ReadOnlyMemoryStream& stream, DataRow& out) const
    {
        return deserialize_object(stream, data_row_schema(), out);
    }

    bool
    StdBinarySerializer::deserialize_dt(ReadOnlyMemoryStream& stream, DataToken& out) const
    {
        return deserialize_object(stream, data_token_schema(), out);
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
