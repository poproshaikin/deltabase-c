#pragma once

#include "accessors.hpp"
#include <functional>
#include <chrono>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <algorithm>
#include <utility>

namespace storage
{
    template <
        typename TKey,
        typename TValue,
        ExternalDataAccessor_c<TKey, TValue> TAccessor,
        TKey (*ExtractKey)(const TValue& value)>
    class EntityCache
    {
    public:
        struct CacheEntry
        {
            TValue value;
            bool is_dirty = false;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;

            CacheEntry() : value(), cached_at(std::chrono::steady_clock::now())
            {
            }

            explicit
            CacheEntry(TValue value)
                : value(std::move(value)), cached_at(std::chrono::steady_clock::now())
            {
            }

            CacheEntry(const CacheEntry& other)
                requires std::is_copy_constructible_v<TValue>
                : value(other.value), is_dirty(other.is_dirty), cached_at(other.cached_at)
            {
            }

            CacheEntry(CacheEntry&& other) noexcept
                : value(std::move(other.value)), is_dirty(other.is_dirty), cached_at(other.cached_at)
            {
            }

            CacheEntry& operator=(const CacheEntry& other)
                requires std::is_copy_assignable_v<TValue>
            {
                value = other.value;
                is_dirty = other.is_dirty;
                cached_at = other.cached_at;
                return *this;
            }

            CacheEntry& operator=(CacheEntry&& other) noexcept
            {
                if (this != &other)
                {
                    if constexpr (std::is_move_assignable_v<TValue>)
                    {
                        value = std::move(other.value);
                    }
                    else
                    {
                        // Destruct and reconstruct with move constructor
                        value.~TValue();
                        new (&value) TValue(std::move(other.value));
                    }
                    is_dirty = other.is_dirty;
                    cached_at = other.cached_at;
                }
                return *this;
            }
        };

    private:
        static constexpr uint64_t max_size = 1000;

        TAccessor accessor_;
        mutable std::unordered_map<TKey, CacheEntry> data_;
        mutable std::shared_mutex mutex_;

    public:
        EntityCache(TAccessor accessor)
            : accessor_(accessor)
        {
        }

        template <typename... Args>
        EntityCache(std::remove_reference<Args&&>... accessor_args)
            : accessor_(std::forward<Args>(accessor_args)...)
        {
        }

        void
        init_with(std::vector<TValue> values)
        {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            for (auto& value : values)
            {
                data_.emplace(ExtractKey(value), CacheEntry(std::move(value)));
            }
        }

        bool
        has(const TKey& key) const noexcept
        { {
                std::shared_lock lock(mutex_);
                if (data_.contains(key))
                {
                    return true;
                }
            }

            if (accessor_.has(key))
                return true;

            return false;
        }

        bool
        has(std::function<bool(const TValue&)> predicate)
        {
            for (TValue& val : *this)
                if (predicate(val))
                    return true;

            return false;
        }

        bool
        has(const TValue& value) const noexcept
        {
            return has(ExtractKey(value));
        }

        TValue&
        get(const TKey& key)
        { {
                std::shared_lock lock(mutex_);
                if (data_.contains(key))
                    return data_.at(key).value;
            }

            if (accessor_.has(key))
            {
                TValue loaded = accessor_.get(key);
                std::unique_lock lock(mutex_);
                auto [iter, inserted] = data_.try_emplace(key, CacheEntry(std::move(loaded)));
                return iter->second.value;
            }

            throw std::runtime_error("Key not found in cache or external storage");
        }

        template <typename U = TValue>
            requires(!std::is_copy_constructible_v<U>)
        void
        put(U&& value)
        {
            std::unique_lock lock(mutex_);

            TKey key = ExtractKey(value);

            if (data_.size() >= max_size && !data_.contains(key))
            {
                auto oldest = std::min_element(
                    data_.begin(),
                    data_.end(),
                    [](const auto& a, const auto& b)
                    {
                        // dirty entries should not be evicted, treat them as "newer"
                        if (a.second.is_dirty && !b.second.is_dirty)
                            return false;
                        if (!a.second.is_dirty && b.second.is_dirty)
                            return true;
                        return a.second.cached_at < b.second.cached_at;
                    }
                );

                if (oldest != data_.end() && !oldest->second.is_dirty)
                    data_.erase(oldest);
                else
                    throw std::runtime_error(
                        "EntityCache::put: cache is full of dirty entries, cannot evict");
            }

            data_.insert_or_assign(key, CacheEntry(std::move(value)));
        }

        template <typename U = TValue>
            requires(std::is_copy_constructible_v<U>)
        void
        put(const U& value)
        {
            std::unique_lock lock(mutex_);

            TKey key = ExtractKey(value);

            if (data_.size() >= max_size && !data_.contains(key))
            {
                auto oldest = std::min_element(
                    data_.begin(),
                    data_.end(),
                    [](const auto& a, const auto& b)
                    {
                        // dirty entries should not be evicted, treat them as "newer"
                        if (a.second.is_dirty && !b.second.is_dirty)
                            return false;
                        if (!a.second.is_dirty && b.second.is_dirty)
                            return true;
                        return a.second.cached_at < b.second.cached_at;
                    }
                );

                if (oldest != data_.end() && !oldest->second.is_dirty)
                    data_.erase(oldest);
                else
                    throw std::runtime_error(
                        "EntityCache::put: cache is full of dirty entries, cannot evict");
            }

            data_.insert_or_assign(key, CacheEntry(value));
        }

        TValue
        remove(const TKey& key)
        {
            std::unique_lock lock(mutex_);
            auto it = data_.find(key);
            if (it != data_.end())
            {
                TValue result = std::move(it->second.value);
                data_.erase(it);
                return result;
            }

            throw std::runtime_error("EntityCache::remove: tried to remove unstaged item");
        }

        TValue
        remove(const TValue& value)
        {
            TKey key = ExtractKey(value);
            return remove(key);
        }

        TValue*
        find_first(std::function<bool(const TValue&)> predicate)
        {
            for (CacheEntry& entry : *this)
                if (predicate(entry.value))
                    return &entry.value;

            return nullptr;
        }

        bool
        remove_first(std::function<bool(const TValue&)> predicate)
        {
            for (CacheEntry& entry : *this)
            {
                if (predicate(entry.value))
                {
                    remove(entry.value);
                    return true;
                }
            }

            return false;
        }

        void
        mark_dirty(const TKey& key)
        {
            if (auto it = data_.find(key); it != data_.end())
                it->second.is_dirty = true;
        }

        void
        mark_dirty(const TValue& value)
        {
            return mark_dirty(ExtractKey(value));
        }

        void
        mark_clean(const TKey& key)
        {
            if (auto it = data_.find(key); it != data_.end())
                it->second.is_dirty = false;
        }

        void
        mark_clean(const TValue& value)
        {
            return mark_clean(ExtractKey(value));
        }

        std::unordered_map<TKey, TValue>
        map_snapshot() const
        {
            std::shared_lock lock(mutex_);

            std::unordered_map<TKey, TValue> result;
            for (const auto& [k, entry] : data_)
                result.emplace(k, entry.value);

            return result;
        }

        // Iterator that exposes CacheEntry (value) instead of map pair
        class Iterator
        {
            using underlying_it_t = typename std::unordered_map<TKey, CacheEntry>::iterator;
            underlying_it_t it_{};

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = CacheEntry;
            using difference_type = std::ptrdiff_t;
            using pointer = CacheEntry*;
            using reference = CacheEntry&;

            Iterator() = default;

            explicit
            Iterator(underlying_it_t it) : it_(it)
            {
            }

            reference
            operator*() const
            {
                return it_->second;
            }

            pointer
            operator->() const
            {
                return std::addressof(it_->second);
            }

            Iterator&
            operator++()
            {
                ++it_;
                return *this;
            }

            Iterator
            operator++(int)
            {
                Iterator tmp = *this;
                ++it_;
                return tmp;
            }

            bool
            operator==(const Iterator& other) const
            {
                return it_ == other.it_;
            }

            bool
            operator!=(const Iterator& other) const
            {
                return it_ != other.it_;
            }
        };

        class ConstIterator
        {
            using underlying_it_t = typename std::unordered_map<TKey, CacheEntry>::const_iterator;
            underlying_it_t it_{};

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const CacheEntry;
            using difference_type = std::ptrdiff_t;
            using pointer = const CacheEntry*;
            using reference = const CacheEntry&;

            ConstIterator() = default;

            explicit
            ConstIterator(underlying_it_t it) : it_(it)
            {
            }

            reference
            operator*() const
            {
                return it_->second;
            }

            pointer
            operator->() const
            {
                return std::addressof(it_->second);
            }

            ConstIterator&
            operator++()
            {
                ++it_;
                return *this;
            }

            ConstIterator
            operator++(int)
            {
                ConstIterator tmp = *this;
                ++it_;
                return tmp;
            }

            bool
            operator==(const ConstIterator& other) const
            {
                return it_ == other.it_;
            }

            bool
            operator!=(const ConstIterator& other) const
            {
                return it_ != other.it_;
            }
        };

        // Range-based for support over CacheEntry values
        Iterator
        begin()
        {
            return Iterator(data_.begin());
        }

        Iterator
        end()
        {
            return Iterator(data_.end());
        }

        ConstIterator
        begin() const
        {
            return ConstIterator(data_.cbegin());
        }

        ConstIterator
        end() const
        {
            return ConstIterator(data_.cend());
        }

        ConstIterator
        cbegin() const
        {
            return ConstIterator(data_.cbegin());
        }

        ConstIterator
        cend() const
        {
            return ConstIterator(data_.cend());
        }

    };
} // namespace storage