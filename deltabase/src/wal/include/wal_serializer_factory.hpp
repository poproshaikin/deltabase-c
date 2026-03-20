//
// Created by poproshaikin on 04.03.26.
//

#ifndef DELTABASE_WAL_SERIALIZER_FACTORY_HPP
#define DELTABASE_WAL_SERIALIZER_FACTORY_HPP
#include "../../types/include/config.hpp"
#include "std_wal_serializer.hpp"
#include "wal_serializer.hpp"

#include <memory>

namespace wal
{
    class WalSerializerFactory
    {
    public:
        std::unique_ptr<IWalSerializer>
        make(types::Config::SerializerType type) const
        {
            switch (type)
            {
            case types::Config::SerializerType::Std:
                return std::make_unique<StdWalSerializer>();
            default:
                throw std::runtime_error(
                    "WalSerializerFactory::make(): invalid serializer type "
                    + std::to_string(static_cast<int>(type))
                );
            }
        }
    };
} // namespace wal

#endif // DELTABASE_WAL_SERIALIZER_FACTORY_HPP
