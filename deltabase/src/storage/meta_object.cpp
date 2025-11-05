#include "include/objects/meta_object.hpp"

#include "../converter/include/converter.hpp"
#include "../misc/include/utils.hpp"

#include <iostream>

namespace storage
{
    // ========== MetaSchema ==========

    MetaSchema::MetaSchema() : id(make_uuid_str())
    {
    }

    bool
    MetaSchema::compare_content(const MetaSchema& other) const
    {
        return id == other.id && name == other.name && db_name == other.db_name;
    }

    bool
    MetaSchema::try_deserialize(const bytes_v& bytes, MetaSchema& out)
    {
        try
        {
            ReadOnlyMemoryStream stream(bytes);

            uint64_t id_len = 0;
            if (stream.read(&id_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.id.resize(id_len);
            if (stream.read(out.id.data(), id_len) != id_len)
                return false;

            uint64_t name_len = 0;
            if (stream.read(&name_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.name.resize(name_len);
            if (stream.read(out.name.data(), name_len) != name_len)
                return false;

            uint64_t db_name_len = 0;
            if (stream.read(&db_name_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.db_name.resize(db_name_len);
            if (stream.read(out.db_name.data(), db_name_len) != db_name_len)
                return false;

            return true;
        }
        catch (std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
            return false;
        }
    }

    bytes_v
    MetaSchema::serialize() const
    {
        bytes_v result;
        MemoryStream stream(result);

        uint64_t id_len = id.size();
        stream.write(&id_len, sizeof(uint64_t));
        stream.write(id.c_str(), id_len);

        uint64_t name_len = name.size();
        stream.write(&name_len, sizeof(uint64_t));
        stream.write(name.c_str(), name_len);

        uint64_t db_name_len = db_name.size();
        stream.write(&db_name_len, sizeof(uint64_t));
        stream.write(db_name.c_str(), db_name_len);

        return result;
    }

    // ========== MetaColumn ==========

    MetaColumn::MetaColumn() : id(make_uuid_str())
    {
    }

    MetaColumn::MetaColumn(const sql::ColumnDefinition& def)
        : MetaColumn(def.name.value,
                     converter::convert_kw_to_vt(def.type.get_detail<sql::SqlKeyword>()),
                     converter::convert_tokens_to_cfs(def.constraints))
    {
    }

    MetaColumn::MetaColumn(const std::string& name, ValueType type, MetaColumnFlags flags)
        : id(make_uuid_str()), name(name), type(type), flags(flags)
    {
    }

    bool
    MetaColumn::try_deserialize(const bytes_v& bytes, MetaColumn& out)
    {
        try
        {
            ReadOnlyMemoryStream stream(bytes);

            uint64_t id_len = 0;
            if (stream.read(&id_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.id.resize(id_len);
            if (stream.read(out.id.data(), id_len) != id_len)
                return false;

            uint64_t table_id_len = 0;
            if (stream.read(&table_id_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.table_id.resize(table_id_len);
            if (stream.read(out.table_id.data(), table_id_len) != table_id_len)
                return false;

            uint64_t name_len = 0;
            if (stream.read(&name_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.name.resize(name_len);
            if (stream.read(out.name.data(), name_len) != name_len)
                return false;

            if (stream.read(&out.type, sizeof(out.type)) != sizeof(out.type))
                return false;
            if (stream.read(&out.flags, sizeof(out.flags)) != sizeof(out.flags))
                return false;

            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bytes_v
    MetaColumn::serialize() const
    {
        bytes_v result;
        MemoryStream stream(result);

        uint64_t id_len = id.size();
        stream.write(&id_len, sizeof(uint64_t));
        stream.write(id.c_str(), id_len);

        uint64_t table_id_len = table_id.size();
        stream.write(&table_id_len, sizeof(uint64_t));
        stream.write(table_id.c_str(), table_id_len);

        uint64_t name_len = name.size();
        stream.write(&name_len, sizeof(uint64_t));
        stream.write(name.c_str(), name_len);

        stream.write(&type, sizeof(ValueType));
        stream.write(&flags, sizeof(MetaColumnFlags));

        return result;
    }

    // ========== MetaTable ==========

    MetaTable::MetaTable() : id(make_uuid_str()), last_rid(0)
    {
    }

    bool
    MetaTable::has_column(const std::string& name) const
    {
        for (const auto& col : columns)
        {
            if (col.name == name)
                return true;
        }
        return false;
    }

    const MetaColumn&
    MetaTable::get_column(const std::string& name) const
    {
        for (const auto& col : columns)
        {
            if (col.name == name)
                return col;
        }
        throw std::runtime_error("Column " + name + " not found in table");
    }

    bool
    MetaTable::compare_content(const MetaTable& other) const
    {
        if (id != other.id || schema_id != other.schema_id || name != other.name ||
            last_rid != other.last_rid)
            return false;

        if (columns.size() != other.columns.size())
            return false;

        if (pages_ids.size() != other.pages_ids.size())
            return false;

        for (size_t i = 0; i < pages_ids.size(); ++i)
        {
            if (pages_ids[i] != other.pages_ids[i])
                return false;
        }

        // TODO: Compare columns content
        return true;
    }

    bool
    MetaTable::try_deserialize(const bytes_v& bytes, MetaTable& out)
    {
        try
        {
            bytes_v result;
            ReadOnlyMemoryStream stream(bytes);

            uint64_t id_len = 0;
            if (stream.read(&id_len, sizeof(uint64_t)) != sizeof(uint64_t))
                return false;
            out.id.resize(id_len);
            if (stream.read(out.id.data(), id_len) != id_len)
                return false;

            uint64_t schema_id_len = 0;
            if (stream.read(&schema_id_len, sizeof(uint64_t) ) != sizeof(uint64_t))
                return false;
            out.schema_id.resize(schema_id_len);
            if (stream.read(out.schema_id.data(), schema_id_len) != schema_id_len)
                return false;

            uint64_t
        }
        catch (...)
        {
            return false;
        }
    }

    bytes_v
    MetaTable::serialize() const
    {
        bytes_v result;
        MemoryStream stream(result);

        uint64_t id_len = id.size();
        stream.write(&id_len, sizeof(uint64_t));
        stream.write(id.c_str(), id_len);

        uint64_t schema_id_len = schema_id.size();
        stream.write(&schema_id_len, sizeof(uint64_t));
        stream.write(schema_id.c_str(), schema_id_len);

        uint64_t name_len = name.size();
        stream.write(&name_len, sizeof(uint64_t));
        stream.write(name.c_str(), name_len);

        uint64_t columns_count = columns.size();
        stream.write(&columns_count, sizeof(uint64_t));

        for (const auto& col : columns)
        {
            auto serialized = col.serialize();
            stream.write(serialized.data(), serialized.size());
        }

        stream.write(&last_rid, sizeof(uint64_t));
        return result;
    }

} // namespace storage
