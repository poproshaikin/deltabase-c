//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_BINARY_SERIALIZER_HPP
#define DELTABASE_BINARY_SERIALIZER_HPP
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/meta_table.hpp"

namespace storage
{
    class IBinarySerializer
    {
    public:
        virtual ~IBinarySerializer() = default;

        virtual types::Bytes
        serialize_mt(const types::MetaTable& table) = 0;

        virtual types::Bytes
        serialize_ms(const types::MetaSchema& schema) = 0;

        virtual types::Bytes
        serialize_mc(const types::MetaColumn& column) = 0;

        virtual bool
        deserialize_mt(const types::Bytes& bytes, types::MetaTable& out) = 0;

        virtual bool
        deserialize_ms(const types::Bytes& bytes, types::MetaSchema& out) = 0;

        virtual bool
        deserialize_mc(const types::Bytes& bytes, types::MetaColumn& out) = 0;
    };
}

#endif //DELTABASE_BINARY_SERIALIZER_HPP