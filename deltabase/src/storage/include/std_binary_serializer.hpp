//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_STD_BINARY_SERIALIZER_HPP
#define DELTABASE_STD_BINARY_SERIALIZER_HPP
#include "binary_serializer.hpp"

namespace storage
{
    class StdBinarySerializer final : public IBinarySerializer
    {
    public:
        types::Bytes
        serialize_mt(const types::MetaTable& table) override;

        types::Bytes
        serialize_ms(const types::MetaSchema& schema) override;

        types::Bytes
        serialize_mc(const types::MetaColumn& column) override;

        types::Bytes
        serialize_dp(const types::DataPage& page) override;

        bool
        deserialize_mt(const types::Bytes& bytes, types::MetaTable& out) override;

        bool
        deserialize_ms(const types::Bytes& bytes, types::MetaSchema& out) override;

        bool
        deserialize_mc(const types::Bytes& bytes, types::MetaColumn& out) override;

        bool
        deserialize_dp(const types::Bytes& bytes, types::DataPage& out) override;
    };
}

#endif //DELTABASE_STD_BINARY_SERIALIZER_HPP