//
// Created by poproshaikin on 15.01.26.
//

#include "binary_serializer_factory.hpp"

#include "std_binary_serializer.hpp"

namespace storage
{
    std::unique_ptr<IBinarySerializer>
    BinarySerializerFactory::make_binary_serializer(types::Config::SerializerType type) const
    {
        switch (type)
        {
        case types::Config::SerializerType::Std:
            return std::make_unique<StdBinarySerializer>();
        default:
            throw std::runtime_error(
                "BinarySerializerFactory::make_binary_serializer: unknown serializer type " + std::to_string(
                    static_cast<int>(type)));
        }
    }

}