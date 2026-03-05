//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_SERIALIZER_FACTORY_HPP
#define DELTABASE_WAL_SERIALIZER_FACTORY_HPP
#include "config.hpp"
#include "wal_serializer.hpp"

#include <memory>

namespace wal
{
    class WalSerializerFactory
    {
    public:
        std::unique_ptr<IWalSerializer>
        make(types::Config::SerializerType type) const;
    };
} // namespace wal

#endif // DELTABASE_WAL_SERIALIZER_FACTORY_HPP
