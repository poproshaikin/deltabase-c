//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_NET_ERROR_HPP
#define DELTABASE_NET_ERROR_HPP

namespace types
{
    enum class NetErrorCode
    {
        SUCCESS = 0,
        PROTOCOL_VIOLATION = 1,
        DB_NOT_EXISTS,
    };
}

#endif // DELTABASE_NET_ERROR_HPP