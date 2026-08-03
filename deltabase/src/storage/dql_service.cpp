//
// Created by poproshaikin on 6/19/26.
//

#include "dql_service.hpp"

#include "BP_index_pager.hpp"
#include "index_bplus_tree.hpp"
#include "../misc/include/utils.hpp"
#include "../misc/include/convert.hpp"

#include <unordered_set>
#include <stdexcept>

namespace storage
{
    using namespace types;
    using namespace misc;

    DQLService::DQLService(BufferPool& buffer_pool)
        : buffer_pool_(buffer_pool)
    {
    }

    DataTable
    DQLService::seq_scan(const MetaTable& mt)
    {
        DataTable dt;
        dt.output_schema = convert(mt);
        dt.rows.reserve(mt.total_rows);

        auto cursor = seq_scan_begin(mt);
        DataRow row;
        while (seq_scan_next(cursor, row))
            dt.rows.push_back(row);

        return dt;
    }

    ScanCursor
    DQLService::seq_scan_begin(const MetaTable& mt)
    {
        const auto pages = buffer_pool_.get_table_data(mt.id);

        ScanCursor cursor{};
        cursor.page = DataPageId::null();
        cursor.slot = 0;
        cursor.chunk_size = 0;
        cursor.initialized = true;

        if (pages.empty())
            return cursor;

        std::unordered_set<DataPageId> referenced_pages;
        referenced_pages.reserve(pages.size());

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            if (page->next != DataPageId::null())
                referenced_pages.insert(page->next);
        }

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            if (!referenced_pages.contains(page->id))
            {
                cursor.page = page->id;
                return cursor;
            }
        }

        for (const auto* page : pages)
        {
            if (!page)
                continue;

            cursor.page = page->id;
            return cursor;
        }

        return cursor;
    }

    bool
    DQLService::seq_scan_next(ScanCursor& cursor, DataRow& out)
    {
        if (!cursor.initialized)
            return false;

        while (cursor.page != DataPageId::null())
        {
            auto* page = buffer_pool_.get_dp(cursor.page);
            if (!page)
            {
                cursor.page = DataPageId::null();
                return false;
            }

            while (cursor.slot < static_cast<int>(page->rows.size()))
            {
                const auto& row = page->rows[static_cast<size_t>(cursor.slot++)];
                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                out = row;
                return true;
            }

            cursor.page = page->next;
            cursor.slot = 0;
        }

        return false;
    }

    DataTable
    DQLService::index_scan(
        const MetaTable& mt,
        const IndexId& index_id,
        const BinaryExpr& condition)
    {
        const MetaIndex* meta_index = nullptr;
        for (const auto& index : mt.indexes)
        {
            if (index.id == index_id)
            {
                meta_index = &index;
                break;
            }
        }

        if (!meta_index)
            throw std::runtime_error("DqlService::index_scan: index is not part of table");

        DataTable dt;
        dt.output_schema = convert(mt);

        auto value_of_node = [&](const AstNode& node, const DataRow& row) -> DataToken
        {
            if (node.type == AstNodeType::IDENTIFIER)
            {
                const auto& col = std::get<SqlToken>(node.value);
                const int64_t col_idx = mt.get_column_idx(col.value);
                if (col_idx < 0)
                    throw std::runtime_error("DqlService::index_scan: column not found");

                return row.tokens[static_cast<size_t>(col_idx)];
            }

            if (node.type == AstNodeType::LITERAL)
                return DataToken(std::get<SqlToken>(node.value));

            throw std::runtime_error("DqlService::index_scan: unsupported condition node");
        };

        auto matches_condition = [&](const DataRow& row) -> bool
        {
            const auto left = value_of_node(*condition.left, row);
            const auto right = value_of_node(*condition.right, row);

            switch (condition.op)
            {
            case AstOperator::EQ:
                return left == right;
            case AstOperator::NEQ:
                return left != right;
            case AstOperator::LT:
                return left < right;
            case AstOperator::LTE:
                return left <= right;
            case AstOperator::GR:
                return left > right;
            case AstOperator::GRE:
                return left >= right;
            default:
                throw std::runtime_error("DqlService::index_scan: unsupported condition op");
            }
        };

        auto append_row_by_ptr = [&](const RowPtr& row_ptr)
        {
            const auto* page = buffer_pool_.get_dp(row_ptr.first);
            if (!page)
                return;

            for (auto it = page->rows.rbegin(); it != page->rows.rend(); ++it)
            {
                const auto& row = *it;
                if (row.id != row_ptr.second)
                    continue;

                if (has_flag(row.flags, DataRowFlags::OBSOLETE))
                    continue;

                if (matches_condition(row))
                    dt.rows.push_back(row);

                return;
            }
        };

        auto is_eq_for_indexed_column = [&](const AstNode* id_node, const AstNode* lit_node) -> bool
        {
            if (id_node->type != AstNodeType::COLUMN_IDENTIFIER ||
                lit_node->type != AstNodeType::LITERAL)
                return false;

            const auto& column_token = std::get<SqlToken>(id_node->value);
            const int64_t col_idx = mt.get_column_idx(column_token.value);
            if (col_idx < 0)
                return false;

            return mt.columns[static_cast<size_t>(col_idx)].id == meta_index->column_id;
        };

        BPIndexPager pager(buffer_pool_, mt.id, index_id);
        IndexBPlusTree tree(pager);

        if (condition.op == AstOperator::EQ)
        {
            if (is_eq_for_indexed_column(condition.left.get(), condition.right.get()))
            {
                const auto key = DataToken(std::get<SqlToken>(condition.right->value));
                auto row_ptr = tree.find(key);
                if (row_ptr.has_value())
                    append_row_by_ptr(row_ptr.value());
                return dt;
            }

            if (is_eq_for_indexed_column(condition.right.get(), condition.left.get()))
            {
                const auto key = DataToken(std::get<SqlToken>(condition.left->value));
                auto row_ptr = tree.find(key);
                if (row_ptr.has_value())
                    append_row_by_ptr(row_ptr.value());
                return dt;
            }
        }

        auto* page = pager.get_page(pager.root_page_id());
        if (!page)
            return dt;

        while (!page->is_leaf)
        {
            const auto& internal = std::get<InternalIndexNode>(page->data);
            if (internal.children.empty())
                return dt;

            page = pager.get_page(internal.children.front());
            if (!page)
                throw std::runtime_error("DqlService::index_scan: broken index tree");
        }

        while (page)
        {
            const auto& leaf = std::get<LeafIndexNode>(page->data);
            for (const auto& row_ptr : leaf.rows)
                append_row_by_ptr(row_ptr);

            if (leaf.next_leaf == 0)
                break;

            page = pager.get_page(leaf.next_leaf);
            if (!page)
                throw std::runtime_error("DqlService::index_scan: broken leaf chain");
        }

        return dt;
    }

    bool
    DQLService::value_exists(
        const MetaTable& mt,
        const std::string& column_name,
        const DataToken& value,
        std::optional<RowId> exclude)
    {
        // Try via index scan
        auto indexes = mt.get_indexes(column_name);
        if (!indexes.empty())
        {
            BPIndexPager pager(buffer_pool_, mt.id, indexes[0]->id);
            IndexBPlusTree tree(pager);
            auto row = tree.find(value);
            if (!row.has_value()) return false;
            if (!exclude.has_value()) return true;
            return exclude.value() != row.value().second;
        }

        // Fallback to seq scan

        int64_t col_idx = mt.get_column_idx(column_name);
        auto cursor = seq_scan_begin(mt);
        DataRow row;
        while (seq_scan_next(cursor, row))
        {
            if (exclude.has_value() && row.id == exclude.value())
                continue;
            if (row.tokens[col_idx] == value)
                return true;
        }

        return false;
    }

    std::vector<DataRow>
    DQLService::get_rows_with_value(
        const MetaTable& mt,
        const std::string& col_name,
        const DataToken& token)
    {
        std::vector<DataRow> rows;
        int64_t col_idx = mt.get_column_idx(col_name);
        auto cursor = seq_scan_begin(mt);

        DataRow row;
        while (seq_scan_next(cursor, row))
            if (row.tokens[col_idx] == token)
                rows.push_back(row);

        return rows;
    }
}