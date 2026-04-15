//
// Created by poproshaikin on 4/13/26.
//

#ifndef DELTABASE_UTILS_HPP
#define DELTABASE_UTILS_HPP
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

struct sockaddr_storage;
namespace net
{
    inline std::string
    get_ip(const sockaddr_storage& addr)
    {
        char buffer[INET6_ADDRSTRLEN];

        if (addr.ss_family == AF_INET)
        {
            auto* v4 = (sockaddr_in*)&addr;
            inet_ntop(AF_INET, &v4->sin_addr, buffer, sizeof(buffer));
        }
        else if (addr.ss_family == AF_INET6)
        {
            auto* v6 = (sockaddr_in6*)&addr;
            inet_ntop(AF_INET6, &v6->sin6_addr, buffer, sizeof(buffer));
        }
        else
        {
            return "unknown";
        }

        return buffer;
    }
}

#endif // DELTABASE_UTILS_HPP
