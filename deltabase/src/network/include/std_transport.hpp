//
// Created by poproshaikin on 4/7/26.
//

#ifndef DELTABASE_STD_TRANSPORT_HPP
#define DELTABASE_STD_TRANSPORT_HPP
#include "transport.hpp"

namespace net
{
    class StdNetTransport : public INetTransport
    {
    public:
        types::Bytes
        read() override;

        void
        write(types::Bytes&& data) override;
    };
} // namespace net

#endif // DELTABASE_STD_TRANSPORT_HPP
