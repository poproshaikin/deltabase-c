//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_TRANSPORT_HPP
#define DELTABASE_TRANSPORT_HPP
#include "typedefs.hpp"

namespace net
{
    class INetTransport
    {
    public:
        virtual ~INetTransport() = default;

        virtual types::Bytes
        read() = 0;

        virtual void
        write(types::Bytes&& data) = 0;
    };
} // namespace net

#endif // DELTABASE_TRANSPORT_HPP
