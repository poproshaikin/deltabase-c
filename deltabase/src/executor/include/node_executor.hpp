//
// Created by poproshaikin on 20.11.25.
//

#ifndef DELTABASE_NODE_EXECUTOR_HPP
#define DELTABASE_NODE_EXECUTOR_HPP
#include "evaluator.hpp"
#include "storage.hpp"
#include "../../types/include/data_row.hpp"
#include "../../types/include/data_table.hpp"
#include "../../types/include/query_plan.hpp"

namespace exq
{

    class INodeExecutor
    {
    public:
        virtual ~INodeExecutor() = default;

        virtual void
        open() = 0;

        virtual bool
        next(types::DataRow& out) = 0;

        virtual void
        close() = 0;

        virtual types::OutputSchema
        output_schema();
    };

    class SeqScanNodeExecutor final : public INodeExecutor
    {
        std::string table_name_;
        std::string schema_name_;
        storage::IStorage& storage_;
        types::DataTable table_;
        uint64_t index_ = 0;

    public:
        explicit
        SeqScanNodeExecutor(
            storage::IStorage& storage,
            const std::string& table_name,
            const std::string& schema_name
        );

        void
        open() override;

        bool
        next(types::DataRow& out) override;

        void
        close() override;

        types::OutputSchema
        output_schema() override;
    };

    class FilterNodeExecutor final : public INodeExecutor
    {
        types::BinaryExpr condition_;
        Evaluator evaluator_;
        const types::MetaTable table_;
        std::unique_ptr<INodeExecutor> child_;

    public:
        explicit
        FilterNodeExecutor(
            const types::MetaTable& table,
            types::BinaryExpr&& condition,
            std::unique_ptr<INodeExecutor> child
        );

        void
        open() override;

        bool
        next(types::DataRow& out) override;

        void
        close() override;

        types::OutputSchema
        output_schema() override;
    };

    class ProjectionNodeExecutor final : public INodeExecutor
    {
        const types::MetaTable table_;
        std::vector<std::string> columns_;
        std::vector<int64_t> indices_;
        std::unique_ptr<INodeExecutor> child_;

    public:
        explicit
        ProjectionNodeExecutor(
            const types::MetaTable& table,
            const std::vector<std::string>& columns,
            std::unique_ptr<INodeExecutor> child
        );

        void
        open() override;

        bool
        next(types::DataRow& out) override;

        void
        close() override;

        types::OutputSchema
        output_schema() override;
    };

    class LimitNodeExecutor final : public INodeExecutor
    {
        const uint64_t limit_ = 0;
        uint64_t current_ = 0;
        std::unique_ptr<INodeExecutor> child_;

    public:
        explicit
        LimitNodeExecutor(uint64_t limit, std::unique_ptr<INodeExecutor> child);

        void
        open() override;

        bool
        next(types::DataRow& out) override;

        void
        close() override;

        types::OutputSchema
        output_schema() override;
    };

    class InsertNodeExecutor final : public INodeExecutor
    {
        std::string table_name_;
        std::string schema_name_;
        storage::IStorage& storage_;
        std::unique_ptr<INodeExecutor> child_;
        bool executed_;

    public:
        explicit
        InsertNodeExecutor(
            const std::string& table_name,
            const std::string& schema_name,
            storage::IStorage& storage,
            std::unique_ptr<INodeExecutor> child
        );

        void
        open() override;

        bool
        next(types::DataRow& out) override;

        void
        close() override;

        types::OutputSchema
        output_schema() override;
    };

    class NodeExecutorFactory
    {
        storage::IStorage& storage_;

    public:
        explicit
        NodeExecutorFactory(storage::IStorage& storage);

        std::unique_ptr<INodeExecutor>
        from_plan(std::unique_ptr<types::IPlanNode>&& node);
    };
}

#endif //DELTABASE_NODE_EXECUTOR_HPP