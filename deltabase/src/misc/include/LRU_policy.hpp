//
// Created by poproshaikin on 3/28/26.
//

#ifndef DELTABASE_LRU_POLICY_HPP
#define DELTABASE_LRU_POLICY_HPP
#include <list>
#include <unordered_map>

namespace cache
{

    template <typename TKey>
    class LRUPolicy
    {
        std::list<TKey> lru_list_;
        std::unordered_map<TKey, typename std::list<TKey>::iterator> pos_;

    public:
        void touch(const TKey& key)
        {
            if (pos_.count(key))
                lru_list_.erase(pos_[key]);
            lru_list_.push_front(key);
            pos_[key] = lru_list_.begin();
        }

        void insert(const TKey& key)
        {
            touch(key);
        }

        TKey evict()
        {
            TKey old = lru_list_.back();
            lru_list_.pop_back();
            pos_.erase(old);
            return old;
        }

        size_t size() const { return lru_list_.size(); }
    };
}

#endif // DELTABASE_LRU_POLICY_HPP
