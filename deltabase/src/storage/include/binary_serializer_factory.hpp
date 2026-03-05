//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#define DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#include "binary_serializer.hpp"
#include "std_binary_serializer.hpp"

namespace storage
{
    class BinarySerializerFactory
    {
    public:
        std::unique_ptr<IBinarySerializer>
        make_binary_serializer(types::Config::SerializerType type) const
        {
            switch (type)
            {
            case types::Config::SerializerType::Std:
                return std::make_unique<StdBinarySerializer>();
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