//
// Created by poproshaikin on 4/1/26.
//

#include "index_bplus_tree.hpp"
namespace storage
{
    std::optional<types::RowPtr>
    IndexBPlusTree::find(const types::DataToken& key)
    {
        auto* leaf_page = find_leaf(key, nullptr);
        auto& leaf = std::get<types::LeafIndexNode>(leaf_page->data);

        for (size_t i = 0; i < leaf.keys.size(); ++i)
            if (compare_token(leaf.keys[i], key) == 0)
                return leaf.rows[i];

        return std::nullopt;
    }

    void
    IndexBPlusTree::insert(const types::DataToken& key, const types::RowPtr& row_ptr)
    {
        std::vector<types::IndexPageId> path;
        auto* leaf_page = find_leaf(key, &path);
        auto& leaf = std::get<types::LeafIndexNode>(leaf_page->data);

        insert_into_leaf(leaf, key, row_ptr);

        if (leaf.keys.size() > max_leaf_keys_)
            split_leaf_and_propagate(*leaf_page, path);

        pager_.mark_dirty();
    }

    void
    IndexBPlusTree::split_leaf_and_propagate(
        types::IndexPage& leaf_page, std::vector<types::IndexPageId>& path
    )
    {
        auto& leaf = std::get<types::LeafIndexNode>(leaf_page.data);
        const size_t mid = leaf.keys.size() / 2;

        auto* right_page = pager_.create_page(true, leaf_page.parent);
        auto& right_leaf = std::get<types::LeafIndexNode>(right_page->data);

        right_leaf.keys.assign(leaf.keys.begin() + static_cast<long>(mid), leaf.keys.end());
        right_leaf.rows.assign(leaf.rows.begin() + static_cast<long>(mid), leaf.rows.end());
        leaf.keys.resize(mid);
        leaf.rows.resize(mid);

        right_leaf.next_leaf = leaf.next_leaf;
        leaf.next_leaf = right_page->id;

        const auto separator = right_leaf.keys.front();
        insert_into_parent(path, leaf_page.id, separator, right_page->id);
    }

    void
    IndexBPlusTree::insert_into_parent(
        std::vector<types::IndexPageId>& path,
        types::IndexPageId left_id,
        const types::DataToken& separator,
        types::IndexPageId right_id
    )
    {
        if (path.size() == 1)
        {
            auto* new_root = pager_.create_page(false, 0);
            auto& r = std::get<types::InternalIndexNode>(new_root->data);
            r.keys.push_back(separator);
            r.children.push_back(left_id);
            r.children.push_back(right_id);

            auto* left = pager_.get_page(left_id);
            auto* right = pager_.get_page(right_id);
            if (!left || !right)
                throw std::runtime_error("IndexBpTree: new root link failed");

            left->parent = new_root->id;
            right->parent = new_root->id;
            pager_.set_root_page_id(new_root->id);
            return;
        }

        const auto parent_id = path[path.size() - 2];
        auto* parent_page = pager_.get_page(parent_id);
        if (!parent_page || parent_page->is_leaf)
            throw std::runtime_error("IndexBpTree: invalid parent");

        auto& parent = std::get<types::InternalIndexNode>(parent_page->data);

        size_t child_pos = 0;
        while (child_pos < parent.children.size() && parent.children[child_pos] != left_id)
            ++child_pos;

        if (child_pos == parent.children.size())
            throw std::runtime_error("IndexBpTree: left child not found in parent");

        parent.keys.insert(parent.keys.begin() + static_cast<long>(child_pos), separator);
        parent.children.insert(
            parent.children.begin() + static_cast<long>(child_pos + 1), right_id
        );

        auto* right = pager_.get_page(right_id);
        if (!right)
            throw std::runtime_error("IndexBpTree: right child not found");
        right->parent = parent_id;

        if (parent.keys.size() > max_internal_keys_)
        {
            path.pop_back();
            split_internal_and_propagate(*parent_page, path);
        }
    }

    void
    IndexBPlusTree::split_internal_and_propagate(
        types::IndexPage& internal_page, std::vector<types::IndexPageId>& path
    )
    {
        auto& node = std::get<types::InternalIndexNode>(internal_page.data);
        const size_t mid = node.keys.size() / 2;
        const auto up_key = node.keys[mid];

        auto* right_page = pager_.create_page(false, internal_page.parent);
        auto& right_node = std::get<types::InternalIndexNode>(right_page->data);

        right_node.keys.assign(node.keys.begin() + static_cast<long>(mid + 1), node.keys.end());
        right_node.children.assign(
            node.children.begin() + static_cast<long>(mid + 1), node.children.end()
        );

        node.keys.resize(mid);
        node.children.resize(mid + 1);

        for (const auto child_id : right_node.children)
        {
            auto* child = pager_.get_page(child_id);
            if (!child)
                throw std::runtime_error("IndexBpTree: child missing during split");
            child->parent = right_page->id;
        }

        insert_into_parent(path, internal_page.id, up_key, right_page->id);
    }

    void
    IndexBPlusTree::insert_into_leaf(
        types::LeafIndexNode& leaf, const types::DataToken& key, const types::RowPtr& row_ptr
    )
    {
        size_t pos = 0;
        while (pos < leaf.keys.size() && compare_token(leaf.keys[pos], key) < 0)
            ++pos;

        leaf.keys.insert(leaf.keys.begin() + static_cast<long>(pos), key);
        leaf.rows.insert(leaf.rows.begin() + static_cast<long>(pos), row_ptr);
    }

    types::IndexPage*
    IndexBPlusTree::find_leaf(const types::DataToken& key, std::vector<types::IndexPageId>* path)
    {
        auto* cur = root();
        while (!cur->is_leaf)
        {
            if (path)
                path->push_back(cur->id);

            auto& node = std::get<types::InternalIndexNode>(cur->data);
            size_t i = 0;
            while (i < node.keys.size() && compare_token(node.keys[i], key) <= 0)
                ++i;

            if (i >= node.children.size())
                throw std::runtime_error("IndexBpTree: broken internal node");

            cur = pager_.get_page(node.children[i]);
            if (!cur)
                throw std::runtime_error("IndexBpTree: child page not found");
        }
        if (path)
        {
            path->push_back(cur->id);
        }

        return cur;
    }

    types::IndexPage*
    IndexBPlusTree::root()
    {
        auto* r = pager_.get_page(pager_.root_page_id());
        if (!r)
            throw std::runtime_error("IndexBpTree: root page not found");
        return r;
    }

    int
    IndexBPlusTree::compare_token(const types::DataToken& a, const types::DataToken& b)
    {
        if (a == b)
            return 0;
        return a < b ? -1 : 1;
    }
} // namespace storage
