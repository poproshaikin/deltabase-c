//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_STD_BINARY_SERIALIZER_HPP
#define DELTABASE_STD_BINARY_SERIALIZER_HPP
#include "binary_serializer.hpp"
#include "../../misc/include/memory_stream.hpp"

namespace storage
{
    class StdBinarySerializer final : public IBinarySerializer
    {
        void
        write_str(const std::string& str, misc::MemoryStream &stream) const;

        bool
        read_str(std::string& str, misc::ReadOnlyMemoryStream& stream) const;

        bool
        has_dynamic_size(types::DataType data_type) const;

        uint64_t
        get_data_type_size(types::DataType data_type) const;

    public:
        misc::MemoryStream
        serialize_mt(const types::MetaTable& table) override;

        misc::MemoryStream
        serialize_ms(const types::MetaSchema& schema) override;

        misc::MemoryStream
        serialize_mc(const types::MetaColumn& column) override;

        misc::MemoryStream
        serialize_dp(const types::DataPage& page) override;

        misc::MemoryStream
        serialize_dr(const types::DataRow& row) override;

        misc::MemoryStream
        serialize_dph(const types::DataPageHeader& header) override;

        misc::MemoryStream
        serialize_dt(const types::DataToken& token) override;

        misc::MemoryStream
        serialize_cfg(const types::Config& db) override;

        bool
        deserialize_mt(misc::ReadOnlyMemoryStream &content, types::MetaTable &out) override;

        bool
        deserialize_ms(misc::ReadOnlyMemoryStream& content, types::MetaSchema& out) override;

        bool
        deserialize_mc(misc::ReadOnlyMemoryStream& content, types::MetaColumn& out) override;

        bool
        deserialize_dp(misc::ReadOnlyMemoryStream& stream, types::DataPage& out) override;

        bool
        deserialize_dph(misc::ReadOnlyMemoryStream& content, types::DataPageHeader& out) override;

        bool
        deserialize_cfg(misc::ReadOnlyMemoryStream& stream, types::Config& out) override;

        bool
        deserialize_dr(misc::ReadOnlyMemoryStream& stream, types::DataRow& out) override;

        bool
        deserialize_dt(misc::ReadOnlyMemoryStream& stream, types::DataToken& out) override;

        uint64_t
        estimate_size(const types::DataRow& row) override;

        uint64_t
        estimate_size(const types::DataToken& token) override;
    };
}

#endif //DELTABASE_STD_BINARY_SERIALIZER_HPP