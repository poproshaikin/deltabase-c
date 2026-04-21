//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_NET_ERROR_HPP
#define DELTABASE_NET_ERROR_HPP

namespace types
{
    enum class NetErrorCode
    {
        // Success status codes
        SUCCESS = 0,
        START_STREAM,
        STREAM_CHUNK,
        END_STREAM,

        // Error status codes
        PROTOCOL_VIOLATION = 100,
        DB_NOT_EXISTS,
        SQL_ERROR,
        UNINITIALIZED_SESSION,
    };
}

#endif // DELTABASE_NET_ERROR_HPP