//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_NET_PROTOCOL_HPP
#define DELTABASE_NET_PROTOCOL_HPP
#include "net_message.hpp"
#include "typedefs.hpp"

namespace net
{
    class INetProtocol
    {
    public:
        virtual ~INetProtocol() = default;

        virtual types::NetMessage
        parse(const types::Bytes& data) const = 0;

        virtual types::Bytes
        encode(const types::NetMessage& msg) const = 0;
    };
} // namespace net

#endif // DELTABASE_NET_PROTOCOL_HPP
