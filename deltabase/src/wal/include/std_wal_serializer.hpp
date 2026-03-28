//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_BINARY_WAL_SERIALIZER_HPP
#define DELTABASE_BINARY_WAL_SERIALIZER_HPP
#include "std_binary_serializer.hpp"
#include "wal_log.hpp"
#include "wal_serializer.hpp"

namespace wal
{
    class StdWalSerializer : public IWalSerializer
    {
        storage::StdBinarySerializer binary_serializer_;

        // ---

        misc::MemoryStream
        serialize(const types::BeginTxnRecord& record) const;

        misc::MemoryStream
        serialize(const types::CommitTxnRecord& record) const;

        misc::MemoryStream
        serialize(const types::RollbackTxnRecord& record) const;

        misc::MemoryStream
        serialize(const types::InsertRecord& record) const;

        misc::MemoryStream
        serialize(const types::UpdateRecord& record) const;

        misc::MemoryStream
        serialize(const types::DeleteRecord& record) const;

        misc::MemoryStream
        serialize(const types::CLRInsertRecord& record) const;

        misc::MemoryStream
        serialize(const types::CLRUpdateRecord& record) const;

        misc::MemoryStream
        serialize(const types::CLRDeleteRecord& record) const;

        misc::MemoryStream
        serialize(const types::CreateSchemaRecord& record) const;

        misc::MemoryStream
        serialize(const types::CreateTableRecord& record) const;

        // ---

    public:
        misc::MemoryStream
        serialize(const types::WALRecord& record) const override;

        bool
        deserialize(misc::ReadOnlyMemoryStream& stream, types::WALRecord& out) override;
    };
} // namespace wal

#endif // DELTABASE_BINARY_WAL_SERIALIZER_HPP
