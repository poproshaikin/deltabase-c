//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_SESSION_HPP
#define DELTABASE_SESSION_HPP
#include "net_message.hpp"

#include <memory>

namespace net
{
    class INetSession
    {
    public:
        virtual ~INetSession() = default;

        virtual void
        handle_message(types::NetMessage msg) = 0;
    };
}

#endif // DELTABASE_SESSION_HPP
