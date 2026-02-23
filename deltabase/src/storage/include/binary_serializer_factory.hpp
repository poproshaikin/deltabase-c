//
// Created by poproshaikin on 15.01.26.
//

#ifndef DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#define DELTABASE_BINARY_SERIALIZER_FACTORY_HPP
#include "binary_serializer.hpp"

namespace storage
{
    class BinarySerializerFactory
    {
    public:
        std::unique_ptr<IBinarySerializer>
        make_binary_serializer(types::Config::SerializerType type) const;
    };
}

#endif //DELTABASE_BINARY_SERIALIZER_FACTORY_HPP