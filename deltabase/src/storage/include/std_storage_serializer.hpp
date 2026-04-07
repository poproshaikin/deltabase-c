//
// Created by poproshaikin on 25.11.25.
//

#ifndef DELTABASE_STD_BINARY_SERIALIZER_HPP
#define DELTABASE_STD_BINARY_SERIALIZER_HPP
#include "../../misc/include/memory_stream.hpp"
#include "storage_serializer.hpp"

namespace storage
{
    class StdStorageSerializer final : public IStorageSerializer
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
        serialize_mt(const types::MetaTable& table) const override;

        misc::MemoryStream
        serialize_ms(const types::MetaSchema& schema) const override;

        misc::MemoryStream
        serialize_mc(const types::MetaColumn& column) const override;

        misc::MemoryStream
        serialize_mi(const types::MetaIndex& index) const override;

        misc::MemoryStream
        serialize_dp(const types::DataPage& page) const override;

        misc::MemoryStream
        serialize_if(const types::IndexFile& file) const override;

        misc::MemoryStream
        serialize_ip(const types::IndexPage& page) const override;

        misc::MemoryStream
        serialize_dr(const types::DataRow& row) const override;

        misc::MemoryStream
        serialize_dt(const types::DataToken& token) const override;

        misc::MemoryStream
        serialize_cfg(const types::Config& db) const override;

        bool
        deserialize_mt(misc::ReadOnlyMemoryStream &content, types::MetaTable &out) const override;

        bool
        deserialize_ms(misc::ReadOnlyMemoryStream& content, types::MetaSchema& out) const override;

        bool
        deserialize_mc(misc::ReadOnlyMemoryStream& content, types::MetaColumn& out) const override;

        bool
        deserialize_mi(misc::ReadOnlyMemoryStream& content, types::MetaIndex& out) const override;

        bool
        deserialize_dp(misc::ReadOnlyMemoryStream& stream, types::DataPage& out) const override;

        bool
        deserialize_if(misc::ReadOnlyMemoryStream& content, types::IndexFile& out) const override;

        bool
        deserialize_ip(misc::ReadOnlyMemoryStream& content, types::IndexPage& out) const override;

        bool
        deserialize_cfg(misc::ReadOnlyMemoryStream& stream, types::Config& out) const override;

        bool
        deserialize_dr(misc::ReadOnlyMemoryStream& stream, types::DataRow& out) const override;

        bool
        deserialize_dt(misc::ReadOnlyMemoryStream& stream, types::DataToken& out) const override;

        uint64_t
        estimate_size(const types::DataRow& row) const override;

        uint64_t
        estimate_size(const types::DataToken& token) const override;
    };
}

#endif //DELTABASE_STD_BINARY_SERIALIZER_HPP