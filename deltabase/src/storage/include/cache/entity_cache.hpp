#pragma once

#include <chrono>
#include <shared_mutex>
#include <unordered_map>

namespace storage {

    template <typename T, typename K, typename V>
    concept ExternalDataAccessor = requires(T getter, const K& key) {
        { getter.has(key) } -> std::same_as<bool>;
        { getter.get(key) } -> std::same_as<V>;
    };

    template<typename K, typename V, ExternalDataAccessor<K, V> Accessor>
    class EntityCache {
        struct CacheEntry {
            V value;
            bool is_dirty = false;
            std::chrono::time_point<std::chrono::steady_clock> cached_at;
        };

        mutable std::shared_mutex mutex_;
        std::unordered_map<K, CacheEntry> data_;
        Accessor accessor_;
        static constexpr uint64_t MAX_SIZE = 1000;

    public:
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
                    return data_.at(key);
                }
            }

            if (accessor_.has(key)) {
                return accessor_.get(key);
            }

            throw std::runtime_error("Key not found in cache or external storage: " + std::to_string(key));
        }

        void
        put(const K& key, const V& value) {

        }
    };
}