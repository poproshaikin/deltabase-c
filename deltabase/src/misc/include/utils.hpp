#ifndef MISC_UTILS_HPP
#define MISC_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
#include <uuid/uuid.h>
#include <stdexcept>

auto
split(const std::string& s, char delimiter, int count = 0) -> std::vector<std::string>;

template <typename Key, typename Value>
auto
get_values(std::unordered_map<Key, Value> map) -> std::vector<Value> {
    std::vector<Value> result;
    result.reserve(map.size());

    for (const auto& pair : map) {
        result.push_back(pair.second);
    }

    return result;
}

template <typename T>
inline auto
vector_to_c(const std::vector<T>& v) -> T* {
    T* arr = new T[v.size()];
    std::copy(v.begin(), v.end(), arr);
    return arr;
}

inline auto
string_vector_to_ptrs(const std::vector<std::string>& v) -> char** {
    char** result = new (std::nothrow) char*[v.size()];
    if (!result)
        return nullptr;

    for (size_t i = 0; i < v.size(); i++) {
        size_t len = v[i].size();
        result[i] = new (std::nothrow) char[len + 1];
        if (!result[i]) {
            for (size_t j = 0; j < i; j++) {
                delete[] result[j];
            }
            delete[] result;
            return nullptr;
        }

        std::memcpy(result[i], v[i].c_str(), len + 1);
    }

    return result;
}

auto
make_c_string(const std::string& str) -> char*;

template <typename T>
inline auto
make_c_arr(const std::vector<T>& vec) -> T* {
    T* arr = new T[vec.size()];
    std::copy(vec.begin(), vec.end(), arr);
    return arr;
}

template <typename T>
inline auto
make_c_ptr_arr(const std::vector<T>& vec) -> T** {
    T** arr = new T*[vec.size()];

    for (size_t i = 0; i < vec.size(); ++i) {
        arr[i] = new T(vec[i]);
    }

    return arr;
}

void
print_ram_usage();

std::string
make_uuid_str();

std::string
make_uuid_str(const uuid_t uuid);

void
parse_uuid_str(const std::string& str, uuid_t uuid);

class MemoryStream 
{
    std::vector<uint8_t>& buffer_;
    mutable size_t position_;
public:
    explicit MemoryStream(std::vector<uint8_t>& buffer) : buffer_(buffer), position_(0)
    {
    }

    size_t
    read(void* dest, size_t count) const
    {
        if (position_ + count > buffer_.size())
            count = buffer_.size() - position_;

        std::memcpy(dest, buffer_.data() + position_, count);
        position_ += count;
        return count;
    }

    void
    write(const void* src, size_t count)
    {
        if (position_ + count > buffer_.size())
            buffer_.resize(position_ + count);

        std::memcpy(buffer_.data() + position_, src, count);
        position_ += count;
    }

    void
    seek(size_t pos) const
    {
        if (pos > buffer_.size())
            throw std::out_of_range("seek past end");

        position_ = pos;
    }

    size_t
    tell() const
    {
        return position_;
    }
};

class ReadOnlyMemoryStream
{
    const std::vector<uint8_t>& buffer_;
    mutable size_t position_;
public:
    explicit ReadOnlyMemoryStream(const std::vector<uint8_t>& buffer) : buffer_(buffer), position_(0)
    {
    }

    size_t
    read(void* dest, size_t count) const
    {
        if (position_ + count > buffer_.size())
            count = buffer_.size() - position_;

        std::memcpy(dest, buffer_.data() + position_, count);
        position_ += count;
        return count;
    }

    void
    seek(size_t pos) const
    {
        if (pos > buffer_.size())
            throw std::out_of_range("seek past end");

        position_ = pos;
    }

    size_t
    tell() const
    {
        return position_;
    }

    size_t
    size() const
    {
        return buffer_.size();
    }

    size_t
    remaining() const
    {
        return buffer_.size() - position_;
    }
};

#endif