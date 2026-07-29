//
// Created by poproshaikin on 09.11.25.
//

#include "engine.hpp"

#include "lexer.hpp"
#include "logger.hpp"
#include "static_storage.hpp"

#include "../misc/include/memory_stream.hpp"
#include "../storage/include/file_utils.hpp"
#include "../storage/include/path.hpp"
#include "../storage/include/std_storage_serializer.hpp"

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

        if (!exists_file(cfg_path))
            throw EngineException("Database " + name + " doesn't exists",
                                  EngineException::Code::DB_NOT_EXISTS);

        ReadOnlyMemoryStream stream(read_file(cfg_path));
        Config cfg;
        if (StdStorageSerializer serializer; !serializer.deserialize_cfg(stream, cfg))
            throw std::runtime_error(
                "StdDbInstance::load_config: failed to load config at path " + cfg_path.string()
            );

        return cfg;
    }

    Engine::Engine() : parser_()
    {
        reset_storage(Config::detached());
    }

    void
    Engine::reset_storage(const Config& config)
    {
        storage_service_provider_ = std::make_unique<StorageServiceProvider>(config);

        parser_.reset();
        planner_ = planner_factory_.make_planner(config, *storage_service_provider_);
        analyzer_ = std::make_unique<exq::SemanticAnalyzer>(config, *storage_service_provider_);

        if (config.db_name.has_value() &&
            !storage_service_provider_->ddl().exists_schema(config.default_schema))
        {
            auto txn = storage_service_provider_->make_txn();
            txn.begin();
            storage_service_provider_->ddl().create_schema(config.default_schema, txn);
            txn.commit();
        }
    }

    void
    Engine::attach_db(const std::string& db_name)
    {
        auto cfg = load_config(db_name, StaticStorage::get_executable_path());
        reset_storage(cfg);
    }

    void
    Engine::create_db(const Config& config)
    {
        reset_storage(config);
    }

    void
    Engine::detach_db()
    {
        planner_.reset();
        analyzer_.reset();
        reset_storage(Config::detached());
    }

    std::unique_ptr<IExecutionResult>
    Engine::make_ok_result(const std::string& tag)
    {
        return std::make_unique<EmptyExecutionResult>();
    }

    std::unique_ptr<IExecutionResult>
    Engine::execute_query(const std::string& query)
    {
        auto tokens = sql::lex(query);

        parser_.reset();
        parser_.set_tokens(tokens);
        auto ast = parser_.parse();

        if (ast.type == AstNodeType::BEGIN)
        {
            if (active_txn_.has_value())
                throw EngineException(
                    "Another transaction is being in progress",
                    EngineException::Code::MULTIPLE_BEGIN);

            active_txn_.emplace(storage_service_provider_->make_txn());
            active_txn_->begin();
            return make_ok_result("BEGIN");
        }
        if (ast.type == AstNodeType::COMMIT)
        {
            if (!active_txn_.has_value())
                throw EngineException("There is no transaction in progress",
                                      EngineException::Code::NO_ACTIVE_TXN);

            active_txn_->commit();
            active_txn_.reset();
            return make_ok_result("COMMIT");
        }
        if (ast.type == AstNodeType::ROLLBACK)
        {
            if (!active_txn_.has_value())
                throw EngineException("There is no transaction in progress",
                                      EngineException::Code::NO_ACTIVE_TXN);
            active_txn_->rollback();
            active_txn_.reset();
            return make_ok_result("ROLLBACK");
        }

        if (ast.type != AstNodeType::CREATE_DATABASE &&
            !storage_service_provider_->config().db_name.has_value())
            throw EngineException("No database attached",
                                  EngineException::Code::DB_NOT_ATTACHED);

        auto analysis = analyzer_->analyze(ast);
        if (!analysis.is_valid)
            throw *analysis.err;

        auto plan = planner_->plan(std::move(ast));

        std::function<void()> on_done;
        bool implicit_txn = !active_txn_.has_value() && plan.needs_txn;
        if (implicit_txn)
        {
            active_txn_.emplace(storage_service_provider_->make_txn());
            active_txn_->begin();
            on_done = [this] { commit_active_txn(); };
        }

        ctx_.txn = active_txn_.has_value() ? &*active_txn_ : nullptr;

        try
        {
            return execute(plan.needs_stream, *plan.root, std::move(on_done));
        }
        catch (...)
        {
            if (implicit_txn)
            {
                active_txn_->rollback();
                active_txn_.reset();
            }
            throw;
        }
    }

    void
    Engine::commit_active_txn()
    {
        active_txn_->commit();
        active_txn_.reset();
    }

    std::unique_ptr<IExecutionResult>
    Engine::execute(bool needs_stream, IPlanNode& root, std::function<void()> on_done)
    {
        auto exec = executor_factory_.from_plan(
            root,
            *storage_service_provider_,
            ctx_);

        if (!needs_stream)
        {
            DataTable result_table;
            DataRow row;

            exec->open();
            while (exec->next(row))
                result_table.rows.push_back(row);
            exec->close();
            result_table.output_schema = exec->output_schema();

            if (on_done)
                on_done();

            return std::make_unique<MaterializedResult>(std::move(result_table));
        }

        return std::make_unique<StreamedResult>(std::move(exec), std::move(on_done));
    }
} // namespace engine