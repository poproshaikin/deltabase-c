//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_STD_NET_PROTOCOL_HPP
#define DELTABASE_STD_NET_PROTOCOL_HPP
#include "protocol.hpp"

namespace net
{
    class StdNetProtocol : public INetProtocol
    {
    public:
        types::Bytes
        encode(const types::NetMessage& msg) override;

        types::Bytes
        parse(const types::Bytes& data) override;
    };
} // namespace net

#endif // DELTABASE_STD_NET_PROTOCOL_HPP
