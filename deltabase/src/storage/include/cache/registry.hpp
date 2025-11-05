#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>
#include <functional>
#include <stdexcept>

namespace storage
{
    template <typename TKey, typename TValue, TKey (*ExtractKey)(const TValue& value)>
    class Registry
    {
        std::unordered_map<TKey, TValue> data_;
        std::unordered_set<TKey> dirty_keys_;

    public:
        Registry() = default;

        // Initialize with a list of values
        void
        init_with(std::vector<TValue> values)
        {
            for (auto& value : values)
            {
                data_.emplace(ExtractKey(value), std::move(value));
            }
        }

        // Check if key exists
        bool
        has(const TKey& key) const noexcept
        {
            return data_.contains(key);
        }

        // Check if value matching predicate exists
        bool
        has(std::function<bool(const TValue&)> predicate) const
        {
            for (const auto& [_, value] : data_)
            {
                if (predicate(value))
                    return true;
            }
            return false;
        }

        // Get value by key (throws if not found)
        TValue&
        get(const TKey& key)
        {
            auto it = data_.find(key);
            if (it == data_.end())
                throw std::runtime_error("Registry::get: key not found");
            return it->second;
        }

        // Get value by key (const version)
        const TValue&
        get(const TKey& key) const
        {
            auto it = data_.find(key);
            if (it == data_.end())
                throw std::runtime_error("Registry::get: key not found");
            return it->second;
        }

        // Get optional value by key
        std::optional<TValue>
        try_get(const TKey& key) const
        {
            auto it = data_.find(key);
            if (it == data_.end())
                return std::nullopt;
            return it->second;
        }

        // Add or update value (copy-constructible types)
        template <typename U = TValue>
            requires(std::is_copy_constructible_v<U>)
        void
        put(const U& value)
        {
            TKey key = ExtractKey(value);
            auto it = data_.find(key);
            if (it != data_.end())
            {
                // If exists, reconstruct in place
                it->second.~TValue();
                new (&it->second) TValue(value);
            }
            else
            {
                // Insert new
                data_.emplace(key, value);
            }
        }

        // Add or update value (move-only types)
        template <typename U = TValue>
            requires(!std::is_copy_constructible_v<U>)
        void
        put(U&& value)
        {
            TKey key = ExtractKey(value);
            auto it = data_.find(key);
            if (it != data_.end())
            {
                // If exists, reconstruct in place
                it->second.~TValue();
                new (&it->second) TValue(std::move(value));
            }
            else
            {
                // Insert new
                data_.emplace(key, std::move(value));
            }
        }

        // Remove value by key
        bool
        remove(const TKey& key)
        {
            return data_.erase(key) > 0;
        }

        bool
        remove(const TValue& value)
        {
            return remove(ExtractKey(value));
        }

        // Find first value matching predicate
        TValue*
        find_first(std::function<bool(const TValue&)> predicate)
        {
            for (auto& [_, value] : data_)
            {
                if (predicate(value))
                    return &value;
            }
            return nullptr;
        }

        // Find first value matching predicate (const version)
        const TValue*
        find_first(std::function<bool(const TValue&)> predicate) const
        {
            for (const auto& [_, value] : data_)
            {
                if (predicate(value))
                    return &value;
            }
            return nullptr;
        }

        // Remove first value matching predicate
        bool
        remove_first(std::function<bool(const TValue&)> predicate)
        {
            for (auto it = data_.begin(); it != data_.end(); ++it)
            {
                if (predicate(it->second))
                {
                    data_.erase(it);
                    return true;
                }
            }
            return false;
        }

        // Get all values as vector
        std::vector<TValue>
        values() const
        {
            std::vector<TValue> result;
            result.reserve(data_.size());
            for (const auto& [_, value] : data_)
            {
                result.push_back(value);
            }
            return result;
        }

        // Get number of elements
        size_t
        size() const noexcept
        {
            return data_.size();
        }

        // Check if empty
        bool
        empty() const noexcept
        {
            return data_.empty();
        }

        // Clear all entries
        void
        clear()
        {
            data_.clear();
            dirty_keys_.clear();
        }

        // Mark value as dirty
        void
        mark_dirty(const TKey& key)
        {
            if (data_.contains(key))
                dirty_keys_.insert(key);
        }

        // Mark value as dirty by value
        void
        mark_dirty(const TValue& value)
        {
            mark_dirty(ExtractKey(value));
        }

        // Mark value as clean
        void
        mark_clean(const TKey& key)
        {
            dirty_keys_.erase(key);
        }

        // Mark value as clean by value
        void
        mark_clean(const TValue& value)
        {
            mark_clean(ExtractKey(value));
        }

        // Check if value is dirty
        bool
        is_dirty(const TKey& key) const
        {
            return dirty_keys_.contains(key);
        }

        // Check if value is dirty by value
        bool
        is_dirty(const TValue& value) const
        {
            return is_dirty(ExtractKey(value));
        }

        // Get all dirty keys
        std::vector<TKey>
        get_dirty_keys() const
        {
            return std::vector<TKey>(dirty_keys_.begin(), dirty_keys_.end());
        }

        // Get all dirty values
        std::vector<TValue>
        get_dirty_values() const
        {
            std::vector<TValue> result;
            result.reserve(dirty_keys_.size());
            for (const auto& key : dirty_keys_)
            {
                auto it = data_.find(key);
                if (it != data_.end())
                    result.push_back(it->second);
            }
            return result;
        }

        // Mark all as clean
        void
        mark_all_clean()
        {
            dirty_keys_.clear();
        }

        // Get count of dirty entries
        size_t
        dirty_count() const noexcept
        {
            return dirty_keys_.size();
        }

        // Iterator support
        auto
        begin()
        {
            return data_.begin();
        }

        auto
        end()
        {
            return data_.end();
        }

        auto
        begin() const
        {
            return data_.begin();
        }

        auto
        end() const
        {
            return data_.end();
        }

        auto
        cbegin() const
        {
            return data_.cbegin();
        }

        auto
        cend() const
        {
            return data_.cend();
        }
    };
} // namespace storage