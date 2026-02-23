//
// Created by poproshaikin on 09.11.25.
//

#include "engine.hpp"

#include "detached_db_instance.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "semantic_analyzer.hpp"
#include "static_storage.hpp"

#include <fstream>

#include "../misc/include/memory_stream.hpp"
#include "../storage/include/file_utils.hpp"
#include "../storage/include/path.hpp"
#include "../storage/include/std_binary_serializer.hpp"
#include "../storage/include/std_db_instance.hpp"

namespace engine
{
    using namespace types;
    using namespace storage;
    using namespace misc;

    Config
    Engine::load_config(const std::string& name, const std::filesystem::path& executable_path) const
    {
        auto data_path = path_data(executable_path);
        auto cfg_path = path_db_meta(data_path, name);

        ReadOnlyMemoryStream stream(read_file(cfg_path));
        Config cfg;
        if (StdBinarySerializer serializer; !serializer.deserialize_cfg(stream, cfg))
            throw std::runtime_error(
                "StdDbInstance::load_config: failed to load config at path " + cfg_path.string()
            );

        return cfg;
    }

    Engine::Engine() : parser_()
    {
        auto cfg = Config::detached();
        auto db = std::make_unique<DetachedDbInstance>(cfg);
        set_db_instance(std::move(db));
    }

    void
    Engine::set_db_instance(std::unique_ptr<IDbInstance> db)
    {
        db_ = std::move(db);

        if (!db_)
        {
            auto config = Config::detached();
            db_ = std::make_unique<DetachedDbInstance>(config);
        }

        auto config = db_->get_config();

        parser_.reset();
        planner_ = planner_factory_.make_planner(config, *db_);
        analyzer_ = std::make_unique<exq::SemanticAnalyzer>(config, *db_);

        if (config.db_name.has_value() && !db_->exists_schema(config.default_schema))
        {
            db_->create_schema(config.default_schema);
        }
    }

    void
    Engine::attach_db(const std::string& db_name)
    {
        auto cfg = load_config(db_name, StaticStorage::get_executable_path());
        auto db = std::make_unique<StdDbInstance>(cfg);
        set_db_instance(std::move(db));
    }

    void
    Engine::create_db(const Config& config)
    {
        Logger::info("Start creating a new database");
        auto db = std::make_unique<StdDbInstance>(config);
        set_db_instance(std::move(db));
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
        if (!analysis.is_valid)
            throw std::runtime_error(std::string("Analysis failed: ") + analysis.err->what());

        auto plan = planner_->plan(std::move(ast));

        auto executor = executor_factory_.from_plan(std::move(plan.root), *db_);

        DataTable result_table;
        DataRow row;

        executor->open();
        while (executor->next(row))
            result_table.rows.push_back(row);
        executor->close();

        return std::make_unique<MaterializedResult>(std::move(result_table));
    }
} // namespace engine