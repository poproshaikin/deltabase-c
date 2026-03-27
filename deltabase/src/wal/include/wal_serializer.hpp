//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_SERIALIZER_HPP
#define DELTABASE_WAL_SERIALIZER_HPP
#include "memory_stream.hpp"
#include "../../types/include/wal_log.hpp"

namespace wal
{
    class IWalSerializer
    {
    public:
        virtual ~IWalSerializer() = default;

        virtual misc::MemoryStream
        serialize(const types::WALRecord& record) const = 0;

        virtual bool
        deserialize(misc::ReadOnlyMemoryStream& stream, types::WALRecord& out) = 0;
    };
}

#endif // DELTABASE_WAL_SERIALIZER_HPP
