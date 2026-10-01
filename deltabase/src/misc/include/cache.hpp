//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_CACHE_HPP
#define DELTABASE_CACHE_HPP

#include <bits/basic_ios.h>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

namespace misc
{
    template <typename TKey, typename TValue, typename TPolicy>
    class Cache
    {
    public:
        struct CacheEntry;
        using iterator = std::unordered_map<TKey, CacheEntry>::iterator;
        using const_iterator = std::unordered_map<TKey, CacheEntry>::const_iterator;

    private:
        std::size_t max_size_;
        std::unordered_map<TKey, CacheEntry> map_;
        TPolicy policy_;

    public:
        Cache(TPolicy policy, std::size_t max_size = 10000) : max_size_(max_size), policy_(policy)
        {
        }

        struct CacheEntry
        {
            TValue value;
            bool dirty;

            explicit CacheEntry(TValue&& value)
                : value(std::move(value)), dirty(false)
            {
            }
        };

        // Returns the evicted entry's value if inserting `key` evicted a dirty
        // victim to make room (the caller is responsible for persisting it --
        // this class does no I/O and must not be asked to, since it may be
        // called while the owner's lock is held).
        std::optional<TValue>
        put(const TKey& key, TValue&& val)
        {
            auto it = map_.find(key);
            if (it != map_.end())
            {
                it->second.value = std::move(val);
                policy_.touch(key);
                return std::nullopt;
            }

            std::optional<TValue> evicted;
            if (map_.size() >= max_size_)
                evicted = evict_one();

            map_.emplace(key, CacheEntry(std::move(val)));
            policy_.insert(key);

            return evicted;
        }

        CacheEntry*
        get(const TKey& key)
        {
            auto it = map_.find(key);
            if (it == map_.end())
                return nullptr;
            policy_.touch(key);
            return &it->second;
        }

        void
        mark_dirty(const TKey& key)
        {
            auto it = map_.find(key);
            if (it == map_.end())
                return;
            policy_.touch(key);
            it->second.dirty = true;
        }

        // Evicts one entry per the policy and returns its value if it was
        // dirty (nullopt otherwise). Does not flush -- see `put` above.
        std::optional<TValue>
        evict_one()
        {
            TKey victim_key = policy_.evict();
            auto it = map_.find(victim_key);
            if (it == map_.end())
                return std::nullopt;

            std::optional<TValue> evicted;
            if (it->second.dirty)
                evicted = std::move(it->second.value);

            map_.erase(it);
            return evicted;
        }

        iterator
        begin()
        {
            return map_.begin();
        }

        iterator
        end()
        {
            return map_.end();
        }

        const_iterator
        begin() const
        {
            return map_.begin();
        }

        const_iterator
        end() const
        {
            return map_.end();
        }

        const_iterator
        cbegin() const
        {
            return map_.cbegin();
        }

        const_iterator
        cend() const
        {
            return map_.cend();
        }

        size_t
        size() const
        {
            return map_.size();
        }
    };
} // namespace buffer

#endif // DELTABASE_CACHE_HPP
