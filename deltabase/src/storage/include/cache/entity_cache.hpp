#pragma once

#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <algorithm>
#include "accessors.hpp"

namespace storage {
    template <
        typename TKey,
        typename TValue,
        ExternalDataAccessor_c<TKey, TValue> TAccessor,
        TKey(*ExtractKey)(const TValue& value)>
    class entity_cache {
        struct cache_entry {
            TValue value;
            bool is_dirty = false;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;

            cache_entry(TValue value)
                : value(value), is_dirty(false), cached_at(std::chrono::steady_clock::now()) {
            }
        };
        static constexpr uint64_t max_size = 1000;

        mutable std::shared_ptr<std::shared_mutex> mutex_;
        std::unordered_map<TKey, cache_entry> data_;
        TAccessor accessor_;

    public:
        entity_cache(TAccessor accessor)
            : accessor_(accessor), mutex_(std::make_shared<std::shared_mutex>()) {
        }

        bool
        has(const TKey& key) const noexcept {
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

        bool 
        has(const TValue& value) const noexcept {
            return has(ExtractKey(value));
        }

        TValue&
        get(const TKey& key) const {
            {
                std::shared_lock lock(mutex_);
                if (data_.contains(key)) {
                    return data_.at(key).value;
                }
            }

            if (accessor_.has(key)) {
                TValue loaded = accessor_.get(key);
                std::unique_lock<std::shared_mutex> lock(*mutex_);
                auto [iter, inserted] = data_.try_emplace(key, cache_entry(std::move(loaded)));
                return iter->second.value;
            }

            throw std::runtime_error("Key not found in cache or external storage");
        }

        void
        put(const TValue& value) {
            std::unique_lock<std::shared_mutex> lock(*mutex_);

            TKey key = ExtractKey(value);
            
            if (data_.size() >= max_size && !data_.contains(key)) {
                auto oldest = std::min_element(data_.begin(), data_.end(),
                    [](const auto& a, const auto& b) {
                        return a.second.cached_at < b.second.cached_at;
                    });
                data_.erase(oldest);
            }
            
            data_[key] = cache_entry(value);
        }

        TValue
        remove(const TKey& key) {
            std::unique_lock<std::shared_mutex> lock(*mutex_);
            auto it = data_.find(key);
            if (it != data_.end()) {
                TValue result = std::move(it->second.value);
                data_.erase(it);
                return result;
            }

            throw std::runtime_error("EntityCache::remove: tried to remove unstaged item");
        }

        TValue
        remove(const TValue& value) {
            TKey key = ExtractKey(value);
            return remove<TKey>(key);
        }

        void
        mark_dirty(const TKey& key) {
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
}