//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_CACHE_HPP
#define DELTABASE_CACHE_HPP

#include <bits/basic_ios.h>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <utility>

namespace misc
{
    template <typename TKey, typename TValue, typename TPolicy>
    class Cache
    {
    public:
        struct CacheEntry;
        using iterator = typename std::unordered_map<TKey, CacheEntry>::iterator;
        using const_iterator = typename std::unordered_map<TKey, CacheEntry>::const_iterator;

    private:
        std::size_t max_size_ = 100;
        std::unordered_map<TKey, CacheEntry> map_;
        TPolicy policy_;

    public:
        Cache(TPolicy policy) : policy_(policy)
        {
        }

        using Flusher = std::function<void(CacheEntry&)>;
        struct CacheEntry
        {
            TValue value;
            bool dirty;
            Flusher flush;

            CacheEntry(TValue&& value, Flusher flush)
                : value(std::move(value)), dirty(false), flush(std::forward<Flusher>(flush))
            {
            }
        };

        void
        put(const TKey& key, TValue&& val, Flusher flush)
        {
            auto it = map_.find(key);
            if (it != map_.end())
            {
                it->second.value = std::move(val);
                policy_.touch(key);
                return;
            }

            if (map_.size() >= max_size_)
                evict_one();

            map_.emplace(key, CacheEntry(std::move(val), std::forward<Flusher>(flush)));
            policy_.insert(key);
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

        void
        evict_one()
        {
            TKey victim_key = policy_.evict();
            auto it = map_.find(victim_key);
            if (it == map_.end())
                return;

            auto& victim = it->second;

            if (victim.dirty)
            {
                victim.flush(victim);
            }

            map_.erase(it);
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
