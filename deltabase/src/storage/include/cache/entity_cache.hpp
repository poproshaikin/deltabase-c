#pragma once

#include <chrono>
#include <unordered_map>

namespace storage {
    template<typename TKey, typename TEntity>
    class EntityCache {
        struct CacheEntry {
            TEntity value;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;
        };

        std::unordered_map<TKey, CacheEntry> data_;
    public:
        const TEntity&
        get(const TKey& key) const;

        void
        cache(const TKey& key, const TEntity& value);

        bool
        has(const TKey& key) const noexcept;
    };
}