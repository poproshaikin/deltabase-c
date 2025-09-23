#pragma once

#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include "accessors.hpp"

namespace storage {

    template<typename K, typename V, ExternalDataAccessor<K, V> Accessor>
    class EntityCache {
        struct CacheEntry {
            V value;
            bool is_dirty = false;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;

            CacheEntry(V value)
                : value(value), is_dirty(false), cached_at(std::chrono::steady_clock::now()) {
            }
        };
        static constexpr uint64_t MAX_SIZE = 1000;

        mutable std::shared_mutex mutex_;
        std::unordered_map<K, CacheEntry> data_;
        Accessor accessor_;

    public:
        EntityCache(Accessor accessor);

        bool
        has(const K& key) const noexcept {
            {
                std::shared_lock lock(mutex_);
                if (data_.contains(key)) {
                    return true;
                }
            }

            if (accessor_.has(key)) {
                return true;
            }

            return false;
        }

        const V&
        get(const K& key) const {
            {
                std::shared_lock lock(mutex_);
                if (data_.contains(key)) {
                    return data_.at(key).value;
                }
            }

            if (accessor_.has(key)) {
                V loaded = accessor_.get(key);
                std::unique_lock<std::shared_mutex> lock(mutex_);
                auto [iter, inserted] = data_.try_emplace(key, CacheEntry(std::move(loaded)));
                return iter->second.value;
            }

            throw std::runtime_error("Key not found in cache or external storage");
        }

        void
        put(const K& key, const V& value) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            
            if (data_.size() >= MAX_SIZE && !data_.contains(key)) {
                auto oldest = std::min_element(data_.begin(), data_.end(),
                    [](const auto& a, const auto& b) {
                        return a.second.cached_at < b.second.cached_at;
                    });
                data_.erase(oldest);
            }
            
            data_[key] = CacheEntry(value);
        }

        void
        mark_dirty(const K& key) {
            if (auto it = data_.find(key); it != data_.end())
                it->second.is_dirty = true;
        }
    };
}