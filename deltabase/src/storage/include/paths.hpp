#pragma once 

#include <string>
#include <filesystem>

static const std::string DB = "data";
static const std::string WAL = "wal";

namespace storage {
    std::filesystem::path
    path_db_wal() {
        return std::filesystem::path(DB) / WAL;
    }
}