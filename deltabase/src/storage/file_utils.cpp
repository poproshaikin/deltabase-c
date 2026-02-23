//
// Created by poproshaikin on 15.01.26.
//

#include "file_utils.hpp"
#include <fstream>

namespace storage
{
    using namespace types;

    Bytes
    read_file(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            throw std::runtime_error("Cannot open file: " + std::string(path));

        std::streamsize size = file.tellg(); // file size
        file.seekg(0, std::ios::beg);        // return to start

        std::vector<uint8_t> buffer(size);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
            throw std::runtime_error("Error reading file: " + std::string(path));

        return buffer;
    }

    void
    write_file(const fs::path& path, const Bytes& content)
    {
        if (!fs::exists(path.parent_path()))
            fs::create_directories(path.parent_path());

        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(content.data()), content.size());
        file.close();
    }
}
