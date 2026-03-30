#include "config.hpp"
#include "io_manager_factory.hpp"
#include "meta_column.hpp"
#include "meta_schema.hpp"
#include "meta_table.hpp"
#include "static_storage.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
    using namespace types;

    struct Args
    {
        std::optional<std::string> db_name;
        std::optional<std::string> schema_name;
        std::optional<std::string> table_name;
        bool show_columns = true;
        bool show_indexes = true;
    };

    std::string
    data_type_to_string(DataType type)
    {
        switch (type)
        {
        case DataType::_NULL:
            return "NULL";
        case DataType::INTEGER:
            return "INTEGER";
        case DataType::REAL:
            return "REAL";
        case DataType::CHAR:
            return "CHAR";
        case DataType::BOOL:
            return "BOOL";
        case DataType::STRING:
            return "STRING";
        default:
            return "UNKNOWN";
        }
    }

    std::string
    column_flags_to_string(MetaColumnFlags flags)
    {
        using Underlying = std::underlying_type_t<MetaColumnFlags>;
        const auto value = static_cast<Underlying>(flags);

        std::vector<std::string> parts;
        if ((value & static_cast<Underlying>(MetaColumnFlags::PK)) != 0)
            parts.emplace_back("PK");
        if ((value & static_cast<Underlying>(MetaColumnFlags::FK)) != 0)
            parts.emplace_back("FK");
        if ((value & static_cast<Underlying>(MetaColumnFlags::AI)) != 0)
            parts.emplace_back("AI");
        if ((value & static_cast<Underlying>(MetaColumnFlags::NN)) != 0)
            parts.emplace_back("NN");
        if ((value & static_cast<Underlying>(MetaColumnFlags::UN)) != 0)
            parts.emplace_back("UN");

        if (parts.empty())
            return "NONE";

        std::ostringstream out;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i != 0)
                out << "|";
            out << parts[i];
        }
        return out.str();
    }

    Args
    parse_args(int argc, char** argv)
    {
        Args args;

        for (int i = 1; i < argc; ++i)
        {
            const std::string opt = argv[i];

            if (opt == "--db")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--db requires a value");
                args.db_name = argv[++i];
                continue;
            }

            if (opt == "--schema")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--schema requires a value");
                args.schema_name = argv[++i];
                continue;
            }

            if (opt == "--table")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--table requires a value");
                args.table_name = argv[++i];
                continue;
            }

            if (opt == "--no-columns")
            {
                args.show_columns = false;
                continue;
            }

            if (opt == "--no-indexes")
            {
                args.show_indexes = false;
                continue;
            }

            if (opt.rfind("--", 0) != 0 && !args.db_name.has_value())
            {
                args.db_name = opt;
                continue;
            }

            throw std::runtime_error("Unknown argument: " + opt);
        }

        return args;
    }

    std::string
    resolve_db_name(const Args& args)
    {
        if (args.db_name.has_value())
            return *args.db_name;

        const auto data_path = misc::StaticStorage::get_executable_path() / "data";
        if (!std::filesystem::exists(data_path))
            throw std::runtime_error(
                "No db name provided and data directory does not exist: " + data_path.string()
            );

        std::vector<std::string> db_names;
        for (const auto& entry : std::filesystem::directory_iterator(data_path))
        {
            if (entry.is_directory())
                db_names.push_back(entry.path().filename().string());
        }

        if (db_names.empty())
            throw std::runtime_error("No db name provided and no databases found in data/");

        if (db_names.size() == 1)
            return db_names.front();

        std::ostringstream msg;
        msg << "No db name provided. Use positional <db_name> or --db <db_name>. Available dbs: ";
        for (size_t i = 0; i < db_names.size(); ++i)
        {
            if (i != 0)
                msg << ", ";
            msg << db_names[i];
        }
        throw std::runtime_error(msg.str());
    }
} // namespace

int
main(int argc, char** argv)
{
    try
    {
        const auto args = parse_args(argc, argv);

        misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());
        const auto db_name = resolve_db_name(args);

        auto cfg = Config::std(db_name);
        storage::IOManagerFactory io_factory;
        auto io = io_factory.make(cfg);
        io->init();

        const auto schemas = io->read_schemas_meta();
        const auto tables = io->read_tables_meta();

        std::unordered_map<std::string, MetaSchema> schema_by_id;
        schema_by_id.reserve(schemas.size());
        for (const auto& schema : schemas)
            schema_by_id.emplace(schema.id.to_string(), schema);

        size_t printed_schemas = 0;
        size_t printed_tables = 0;
        size_t printed_columns = 0;
        size_t printed_indexes = 0;

        std::cout << "=== META DUMP ===\n";
        std::cout << "db=" << db_name << "\n\n";

        for (const auto& schema : schemas)
        {
            if (args.schema_name.has_value() && schema.name != *args.schema_name)
                continue;

            std::cout << "SCHEMA " << schema.name << " id=" << schema.id.to_string() << "\n";
            printed_schemas += 1;

            for (const auto& table : tables)
            {
                if (table.schema_id != schema.id)
                    continue;
                if (args.table_name.has_value() && table.name != *args.table_name)
                    continue;

                std::cout << "  TABLE " << table.name
                          << " id=" << table.id.to_string()
                          << " last_rid=" << table.last_rid << "\n";
                printed_tables += 1;

                if (args.show_columns)
                {
                    for (const auto& column : table.columns)
                    {
                        std::cout << "    COLUMN " << column.name
                                  << " id=" << column.id.to_string()
                                  << " type=" << data_type_to_string(column.type)
                                  << " flags=" << column_flags_to_string(column.flags)
                                  << "\n";
                        printed_columns += 1;
                    }
                }

                if (args.show_indexes)
                {
                    for (const auto& index : table.indexes)
                    {
                        std::cout << "    INDEX " << index.name
                                  << " id=" << index.id.to_string()
                                  << " table_id=" << index.table_id.to_string()
                                  << " column_id=" << index.column_id.to_string()
                                  << " root_page_id=" << index.root_page_id.to_string()
                                  << " key_type=" << data_type_to_string(index.key_type)
                                  << " unique=" << (index.is_unique ? "true" : "false")
                                  << "\n";
                        printed_indexes += 1;
                    }
                }
            }

            std::cout << "\n";
        }

        if (printed_schemas == 0)
            std::cout << "No matching schemas found.\n";

        std::cout << "=== META SUMMARY ===\n";
        std::cout << "schemas=" << printed_schemas
                  << " tables=" << printed_tables
                  << " columns=" << printed_columns
                  << " indexes=" << printed_indexes << "\n";

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "mt_dump error: " << ex.what() << "\n";
        return 1;
    }
}
