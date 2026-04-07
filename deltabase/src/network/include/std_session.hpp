//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_STD_NET_SESSION_HPP
#define DELTABASE_STD_NET_SESSION_HPP
#include "engine.hpp"
#include "protocol.hpp"
#include "session.hpp"

namespace net
{
    class StdNetSession : public INetSession
    {
        engine::Engine engine_;
        INetProtocol& protocol_;
        INetTransport& transport_;
    };
}

#endif // DELTABASE_STD_NET_SESSION_HPP
