#!/bin/bash

# Найти все C++ файлы и применить clang-tidy
find src -type f \( -name "*.cpp" -o -name "*.hpp" \) -exec clang-tidy \
    --checks="readability-identifier-naming,modernize-*,-modernize-use-trailing-return-type" \
    --fix \
    --format-style=file \
    {} \; \
    -- -std=c++20 -I./src

echo "clang-tidy completed for all files!"
