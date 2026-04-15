//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#define DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#include "std_storage_serializer.hpp"
#include "storage_serializer.hpp"

namespace storage
{
    class StorageSerializerFactory
    {
    public:
        std::unique_ptr<IStorageSerializer>
        make_binary_serializer(types::Config::SerializerType type) const
        {
            switch (type)
            {
            case types::Config::SerializerType::Std:
                return std::make_unique<StdStorageSerializer>();
            default:
                throw std::runtime_error(
                    "BinarySerializerFactory::make_binary_serializer: unknown serializer type " +
                    std::to_string(static_cast<int>(type))
                );
            }
        }
    };
} // namespace storage

#endif // DELTABASE_BINARY_SERIALIZER_FACTORY_HPP