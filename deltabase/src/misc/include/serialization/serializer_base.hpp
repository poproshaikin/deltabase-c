#ifndef DELTABASE_SERIALIZER_BASE_HPP
#define DELTABASE_SERIALIZER_BASE_HPP

#include "write_read_cursor.hpp"

#include <memory>
#include <vector>

namespace misc::serialization
{
    template <typename TObject>
    struct Schema;

    template <typename TObject>
    bool
    write_by_schema(const TObject& object, const Schema<TObject>& object_schema, WriteCursor& cursor);

    template <typename TObject>
    bool
    read_by_schema(ReadCursor& cursor, const Schema<TObject>& object_schema, TObject& out);

    template <typename TObject>
    class ISchemaField
    {
    public:
        virtual ~ISchemaField() = default;

        virtual bool
        write(const TObject& object, WriteCursor& cursor) const = 0;

        virtual bool
        read(ReadCursor& cursor, TObject& object) const = 0;
    };

    template <typename TObject>
    struct Schema
    {
        std::vector<std::unique_ptr<ISchemaField<TObject>>> fields;
    };

    template <typename TObject>
    bool
    write_by_schema(const TObject& object, const Schema<TObject>& object_schema, WriteCursor& cursor)
    {
        for (const auto& field : object_schema.fields)
        {
            if (!field->write(object, cursor))
                return false;
        }

        return true;
    }

    template <typename TObject>
    bool
    read_by_schema(ReadCursor& cursor, const Schema<TObject>& object_schema, TObject& out)
    {
        for (const auto& field : object_schema.fields)
        {
            if (!field->read(cursor, out))
                return false;
        }

        return true;
    }

    class SerializerBase
    {
    protected:
        template <typename TObject>
        bool
        fulfill_and_write_schema(
            const TObject& object,
            const Schema<TObject>& object_schema,
            WriteCursor& cursor
        ) const
        {
            return write_by_schema(object, object_schema, cursor);
        }

        template <typename TObject
>
        bool
        read_by_schema(
            ReadCursor& cursor,
            const Schema<TObject>& object_schema,
            TObject& out
        ) const
        {
            return serialization::read_by_schema(cursor, object_schema, out);
        }
    };
} // namespace misc::serialization

#endif // DELTABASE_SERIALIZER_BASE_HPP
