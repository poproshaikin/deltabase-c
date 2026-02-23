//
// Created by poproshaikin on 10.12.25.
//

#ifndef DELTABASE_STATIC_STORAGE_HPP
#define DELTABASE_STATIC_STORAGE_HPP
#include <filesystem>
#include <iostream>

namespace misc
{
    class StaticStorage
    {
        static inline std::filesystem::path executable_path;

    public:
        static const std::filesystem::path&
        get_executable_path()
        {
            return executable_path;
        }

        static void
        set_executable_path(const std::filesystem::path& path)
        {
            executable_path = path;
        }
    };
} // namespace misc

#endif // DELTABASE_STATIC_STORAGE_HPP