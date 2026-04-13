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
        types::Bytes
        encode(const types::PingNetMessage& msg) const;
        types::Bytes
        encode(const types::PongNetMessage& msg) const;
        types::Bytes
        encode(const types::QueryNetMessage& msg) const;
        types::Bytes
        encode(const types::CreateDbNetMessage& msg) const;
        types::Bytes
        encode(const types::AttachDbNetMessage& msg) const;
        types::Bytes
        encode(const types::CloseNetMessage& msg) const;

    public:
        types::Bytes
        encode(const types::NetMessage& msg) const override;

        types::NetMessage
        parse(const types::Bytes& data) const override;
    };
} // namespace net

#endif // DELTABASE_STD_NET_PROTOCOL_HPP
