//
// Created by poproshaikin on 09.11.25.
//

#include "engine.hpp"

#include "lexer.hpp"

#include <fstream>

#include "../misc/include/memory_stream.hpp"
#include "../executor/include/std_planner.hpp"
#include "../storage/include/std_db_instance.hpp"
#include "../storage/include/std_binary_serializer.hpp"
#include "../storage/include/path.hpp"

namespace engine
{
    using namespace types;
    using namespace storage;

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

        misc::ReadOnlyMemoryStream stream(read_file(cfg_path));
        Config cfg;
        if (StdBinarySerializer serializer; !serializer.deserialize_cfg(stream, cfg))
            throw std::runtime_error(
                "StdDbInstance::load_config: failed to load config at path " + cfg_path.string());

        return cfg;
    }


    void
    Engine::init(const Config& cfg)
    {
        db_ = std::make_unique<StdDbInstance>(cfg);
        executor_factory_ = exq::NodeExecutorFactory();

        init_planner(cfg.planner_type);
    }

    void
    Engine::init_planner(Config::PlannerType planner_type)
    {
        assert(db_ != nullptr);

        switch (planner_type)
        {
        case Config::PlannerType::Std:
            planner_ = std::make_unique<exq::StdPlanner>(exq::StdPlanner(db_->get_config(), *db_));
            break;
        default:
            throw std::runtime_error(
                "Engine::Engine: unknown planner type: " + std::to_string(
                    static_cast<int>(db_->get_config().planner_type)));
        }
    }

    Engine::Engine(const Config& cfg) : parser_()
    {
        init(cfg);
    }

    void
    Engine::attach_db(const std::string& db_name)
    {
        std::filesystem::path current_path = std::filesystem::current_path();
        init(load_config(db_name, current_path));
    }

    void
    Engine::create_db(const Config& config)
    {
        db_ = std::make_unique<StdDbInstance>(config);
        init_planner(config.planner_type);
    }

    std::unique_ptr<IExecutionResult>
    Engine::execute_query(const std::string& query)
    {
        auto tokens = sql::lex(query);

        parser_.reset();
        parser_.set_tokens(tokens);
        auto ast = parser_.parse();
        auto plan = planner_->plan(std::move(ast));

        if (!plan.db_specific)
            return execute_generic(std::move(plan));

        auto executor = executor_factory_.from_plan(std::move(plan.root));

        DataTable table;
        while ()
    }
}