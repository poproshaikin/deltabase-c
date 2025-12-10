//
// Created by poproshaikin on 09.11.25.
//

#include "engine.hpp"

#include "lexer.hpp"
#include "semantic_analyzer.hpp"

#include <fstream>

#include "../misc/include/memory_stream.hpp"
#include "../storage/include/std_db_instance.hpp"
#include "../storage/include/std_binary_serializer.hpp"
#include "../storage/include/path.hpp"

namespace engine
{
    using namespace types;
    using namespace storage;
    using namespace misc;

    static Bytes
    read_file(const std::filesystem::path& path)
    {
        std::uintmax_t size = std::filesystem::file_size(path);
        Bytes buffer(size);

        std::ifstream f(path, std::ios::binary);
        f.read(reinterpret_cast<char*>(buffer.data()), size);

        return buffer;
    }

    Config
    Engine::load_config(const std::string& name, const std::filesystem::path& current_path) const
    {
        auto cfg_path = path_db_meta(current_path, name);

        ReadOnlyMemoryStream stream(read_file(cfg_path));
        Config cfg;
        if (StdBinarySerializer serializer; !serializer.deserialize_cfg(stream, cfg))
            throw std::runtime_error(
                "StdDbInstance::load_config: failed to load config at path " + cfg_path.string());

        return cfg;
    }

    void
    Engine::init_db(std::unique_ptr<IDbInstance> db)
    {
        if (!db)
            throw std::runtime_error("Engine::init_db: db cannot be null");

        if (db_)
        {
            planner_.reset();
            db_.reset();
        }

        db_ = std::move(db);
        planner_ = planner_factory_.make_planner(db_->get_config(), *db_);
        analyzer_ = std::make_unique<exq::SemanticAnalyzer>(*db_);
    }

    std::unique_ptr<IExecutionResult>
    Engine::execute_generic(QueryPlan&& plan)
    {
        switch (plan.type)
        {
        case QueryPlan::Type::CREATE_DB:
        {
            const auto& create_db_node = static_cast<const CreateDbPlanNode&>(*plan.root);
            create_db(Config(create_db_node.db_name));
            return std::make_unique<EmptyExecutionResult>();
        }
        default:
            throw std::runtime_error(
                "Engine::execute_generic: unknown generic query " + std::to_string(
                    static_cast<int>(plan.type)
                )
            );
        }
    }

    Engine::Engine()
        : parser_(),
          planner_(nullptr),
          db_(nullptr),
          executor_factory_(),
          planner_factory_()
    {
    }

    void
    Engine::attach_db(const std::string& db_name)
    {
        std::filesystem::path current_path = std::filesystem::current_path();

        auto cfg = load_config(db_name, current_path);
        auto db = std::make_unique<StdDbInstance>(cfg);
        init_db(std::move(db));
    }

    void
    Engine::create_db(const Config& config)
    {
        auto db = std::make_unique<StdDbInstance>(config);
        init_db(std::move(db));
    }

    void
    Engine::detach_db()
    {
        planner_.reset();
        db_.reset();
        analyzer_.reset();
    }


    std::unique_ptr<IExecutionResult>
    Engine::execute_query(const std::string& query)
    {
        auto tokens = sql::lex(query);

        parser_.reset();
        parser_.set_tokens(tokens);
        auto ast = parser_.parse();

        auto analysis = analyzer_->analyze(ast);

        auto plan = planner_->plan(std::move(ast));

        if (!plan.db_specific)
            return execute_generic(std::move(plan));

        auto executor = executor_factory_.from_plan(std::move(plan.root), *db_);

        DataTable result_table;
        DataRow row;

        executor->open();
        while (executor->next(row))
            result_table.rows.push_back(row);
        executor->close();

        return std::make_unique<MaterializedResult>(std::move(result_table));
    }
}