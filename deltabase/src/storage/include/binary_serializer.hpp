//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_BINARY_SERIALIZER_HPP
#define DELTABASE_BINARY_SERIALIZER_HPP
#include "../../misc/include/memory_stream.hpp"
#include "../../types/include/meta_schema.hpp"
#include "../../types/include/meta_table.hpp"
#include "../../types/include/data_page.hpp"
#include "../../types/include/config.hpp"

namespace storage
{
    class IBinarySerializer
    {
    public:
        virtual ~IBinarySerializer() = default;

        virtual misc::MemoryStream
        serialize_mt(const types::MetaTable& table) const = 0;

        virtual misc::MemoryStream
        serialize_ms(const types::MetaSchema& schema) const = 0;

        virtual misc::MemoryStream
        serialize_mc(const types::MetaColumn& column) const = 0;

        virtual misc::MemoryStream
        serialize_mi(const types::MetaIndex& index) const = 0;

        virtual misc::MemoryStream
        serialize_cfg(const types::Config& db) const = 0;

        virtual misc::MemoryStream
        serialize_dp(const types::DataPage& page) const = 0;

        virtual misc::MemoryStream
        serialize_dr(const types::DataRow& row) const = 0;

        virtual misc::MemoryStream
        serialize_dt(const types::DataToken& token) const = 0;

        virtual bool
        deserialize_mt(misc::ReadOnlyMemoryStream& content, types::MetaTable &out) const = 0;

        virtual bool
        deserialize_ms(misc::ReadOnlyMemoryStream& content, types::MetaSchema& out) const = 0;

        virtual bool
        deserialize_mc(misc::ReadOnlyMemoryStream& content, types::MetaColumn& out) const = 0;

        virtual bool
        deserialize_mi(misc::ReadOnlyMemoryStream& content, types::MetaIndex& out) const = 0;

        virtual bool
        deserialize_dp(misc::ReadOnlyMemoryStream& content, types::DataPage& out) const = 0;

        virtual bool
        deserialize_cfg(misc::ReadOnlyMemoryStream& content, types::Config& out) const = 0;

        virtual bool
        deserialize_dr(misc::ReadOnlyMemoryStream& content, types::DataRow& out) const = 0;

        virtual bool
        deserialize_dt(misc::ReadOnlyMemoryStream& content, types::DataToken& out) const = 0;

        virtual uint64_t
        estimate_size(const types::DataRow& row) const = 0;

        virtual uint64_t
        estimate_size(const types::DataToken& token) const = 0;
    };
}

#endif //DELTABASE_BINARY_SERIALIZER_HPP