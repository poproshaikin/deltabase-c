//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_SERIALIZER_HPP
#define DELTABASE_WAL_SERIALIZER_HPP
#include "file_wal_io_manager.hpp"
#include "memory_stream.hpp"

namespace wal
{
    class IWalSerializer
    {
    public:
        virtual ~IWalSerializer() = default;

        virtual misc::MemoryStream
        serialize(const types::WalRecord& record) const = 0;
    };
}

#endif // DELTABASE_WAL_SERIALIZER_HPP
