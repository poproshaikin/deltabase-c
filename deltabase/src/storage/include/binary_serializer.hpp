//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_BINARY_SERIALIZER_HPP
#define DELTABASE_BINARY_SERIALIZER_HPP
#include "memory_stream.hpp"
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
        serialize_mt(const types::MetaTable& table) = 0;

        virtual misc::MemoryStream
        serialize_ms(const types::MetaSchema& schema) = 0;

        virtual misc::MemoryStream
        serialize_mc(const types::MetaColumn& column) = 0;

        virtual misc::MemoryStream
        serialize_cfg(const types::Config& db) = 0;

        virtual misc::MemoryStream
        serialize_dph(const types::DataPageHeader& header) = 0;

        virtual misc::MemoryStream
        serialize_dp(const types::DataPage& page) = 0;

        virtual misc::MemoryStream
        serialize_dr(const types::DataRow& row) = 0;

        virtual misc::MemoryStream
        serialize_dt(const types::DataToken& token) = 0;

        virtual bool
        deserialize_mt(misc::ReadOnlyMemoryStream& content, types::MetaTable &out) = 0;

        virtual bool
        deserialize_ms(misc::ReadOnlyMemoryStream& content, types::MetaSchema& out) = 0;

        virtual bool
        deserialize_mc(misc::ReadOnlyMemoryStream& content, types::MetaColumn& out) = 0;

        virtual bool
        deserialize_dph(misc::ReadOnlyMemoryStream& content, types::DataPageHeader& out) = 0;

        virtual bool
        deserialize_dp(misc::ReadOnlyMemoryStream& content, types::DataPage& out) = 0;

        virtual bool
        deserialize_cfg(misc::ReadOnlyMemoryStream& content, types::Config& out) = 0;

        virtual bool
        deserialize_dr(misc::ReadOnlyMemoryStream& content, types::DataRow& out) = 0;

        virtual bool
        deserialize_dt(misc::ReadOnlyMemoryStream& content, types::DataToken& out) = 0;
    };
}

#endif //DELTABASE_BINARY_SERIALIZER_HPP