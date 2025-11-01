#pragma once

#include <chrono>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include "accessors.hpp"

namespace storage
{
    template <
        typename TKey,
        typename TValue,
        ExternalDataAccessor_c<TKey, TValue> TAccessor,
        TKey (*ExtractKey)(const TValue& value)>
    class EntityCache
    {
        struct CacheEntry
        {
            TValue value;
            bool is_dirty = false;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;

            CacheEntry(TValue value)
                : value(value), is_dirty(false), cached_at(std::chrono::steady_clock::now())
            {
            }
        };
        
        static constexpr uint64_t max_size = 1000;

        TAccessor accessor_;
        std::unordered_map<TKey, CacheEntry> data_;
        mutable std::shared_ptr<std::shared_mutex> mutex_;

    public:
        EntityCache(TAccessor accessor)
            : accessor_(accessor), mutex_(std::make_shared<std::shared_mutex>())
        {
        }

        template <typename... Args>
        EntityCache(std::remove_reference<Args&&>... accessor_args)
            : accessor_(std::forward<Args>(accessor_args)...), mutex_(std::make_shared<std::shared_mutex>())
        {
        }

        void
        init_with(std::vector<TValue> values)
        {
            std::unique_lock<std::shared_mutex> lock(*mutex_);
            for (const TValue& value : values)
            {
                data_.emplace(ExtractKey(value), CacheEntry(std::move(values)));
            }
        }

        // Iterator that exposes CacheEntry (value) instead of map pair
        class Iterator {
            using underlying_it_t = typename std::unordered_map<TKey, CacheEntry>::iterator;
            underlying_it_t it_{};
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = CacheEntry;
            using difference_type = std::ptrdiff_t;
            using pointer = CacheEntry*;
            using reference = CacheEntry&;

            Iterator() = default;
            explicit Iterator(underlying_it_t it) : it_(it) {}

            reference operator*() const { return it_->second; }
            pointer operator->() const { return std::addressof(it_->second); }

            Iterator& operator++() { ++it_; return *this; }
            Iterator operator++(int) { Iterator tmp = *this; ++it_; return tmp; }

            bool operator==(const Iterator& other) const { return it_ == other.it_; }
            bool operator!=(const Iterator& other) const { return it_ != other.it_; }
        };

        class ConstIterator {
            using underlying_it_t = typename std::unordered_map<TKey, CacheEntry>::const_iterator;
            underlying_it_t it_{};
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = const CacheEntry;
            using difference_type = std::ptrdiff_t;
            using pointer = const CacheEntry*;
            using reference = const CacheEntry&;

            ConstIterator() = default;
            explicit ConstIterator(underlying_it_t it) : it_(it) {}

            reference operator*() const { return it_->second; }
            pointer operator->() const { return std::addressof(it_->second); }

            ConstIterator& operator++() { ++it_; return *this; }
            ConstIterator operator++(int) { ConstIterator tmp = *this; ++it_; return tmp; }

            bool operator==(const ConstIterator& other) const { return it_ == other.it_; }
            bool operator!=(const ConstIterator& other) const { return it_ != other.it_; }
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

        bool
        has(const TKey& key) const noexcept
        {
            {
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
        has(const TValue& value) const noexcept
        {
            return has(ExtractKey(value));
        }

        TValue&
        get(const TKey& key) const
        {
            {
                std::shared_lock lock(mutex_);
                if (data_.contains(key))
                    return data_.at(key).value;
            }

            if (accessor_.has(key))
            {
                TValue loaded = accessor_.get(key);
                std::unique_lock<std::shared_mutex> lock(*mutex_);
                auto [iter, inserted] = data_.try_emplace(key, CacheEntry(std::move(loaded)));
                return iter->second.value;
            }

            throw std::runtime_error("Key not found in cache or external storage");
        }

        void
        put(const TValue& value)
        {
            std::unique_lock<std::shared_mutex> lock(*mutex_);

            TKey key = ExtractKey(value);

            if (data_.size() >= max_size && !data_.contains(key))
            {
                auto oldest = std::min_element(
                    data_.begin(),
                    data_.end(),
                    [](const auto& a, const auto& b)
                    { return a.second.cached_at < b.second.cached_at; }
                );
                data_.erase(oldest);
            }

            data_[key] = CacheEntry(value);
        }

        TValue
        remove(const TKey& key)
        {
            std::unique_lock<std::shared_mutex> lock(*mutex_);
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
            return remove<TKey>(key);
        }

        void
        mark_dirty(const TKey& key)
        {
            if (auto it = data_.find(key); it != data_.end())
                it->second.is_dirty = true;
        }

        void 
        mark_dirty(const TValue& value) {
            return mark_dirty(ExtractKey(value));
        }

        std::unordered_map<TKey, TValue>
        map_snapshot() const {
            std::shared_lock lock(*mutex_);
            std::unordered_map<TKey, TValue> result;
            for (const auto& [k, entry] : data_) {
                result.emplace(k, entry.value);
            }
            return result;
        }
    };
} // namespace storage