//
// Created by poproshaikin on 10.12.25.
//

#ifndef DELTABASE_STATIC_STORAGE_HPP
#define DELTABASE_STATIC_STORAGE_HPP
#include <filesystem>

namespace misc
{
    class StaticStorage
    {
    public:
        static inline std::filesystem::path executable_path;
    };
}

#endif //DELTABASE_STATIC_STORAGE_HPP