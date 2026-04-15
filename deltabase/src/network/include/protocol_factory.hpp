//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_PROTOCOL_FACTORY_HPP
#define DELTABASE_PROTOCOL_FACTORY_HPP
#include "config.hpp"
#include "protocol.hpp"
#include "std_protocol.hpp"

#include <memory>

namespace net
{
    class NetProtocolFactory
    {
    public:
        std::unique_ptr<INetProtocol>
        make(types::Config::NetProtocolType type) const
        {
            switch (type)
            {
            case types::Config::NetProtocolType::Std:
                return std::make_unique<StdNetProtocol>();
            default:
                throw std::runtime_error(
                    "Unknown net protocol type: " + std::to_string(static_cast<int>(type))
                );
            }
        }
    };
} // namespace net

#endif // DELTABASE_PROTOCOL_FACTORY_HPP
