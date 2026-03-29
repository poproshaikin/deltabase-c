#include "config.hpp"
#include "data_row.hpp"
#include "io_manager_factory.hpp"
#include "meta_schema.hpp"
#include "meta_table.hpp"
#include "static_storage.hpp"

#include <cstdint>
#include <cstring>
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
        std::optional<std::string> page_id;
    };

    std::string
    to_hex(const Bytes& bytes)
    {
        std::ostringstream out;
        out << "0x";
        for (uint8_t b : bytes)
            out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);

        return out.str();
    }

    std::string
    token_to_string(const DataToken& token)
    {
        std::ostringstream out;
        out << "{";

        switch (token.type)
        {
        case DataType::_NULL:
            out << "null";
            break;
        case DataType::INTEGER:
            if (token.bytes.size() == sizeof(int))
            {
                int value = 0;
                std::memcpy(&value, token.bytes.data(), sizeof(int));
                out << value;
            }
            else
            {
                out << "int?" << to_hex(token.bytes);
            }
            break;
        case DataType::REAL:
            if (token.bytes.size() == sizeof(double))
            {
                double value = 0;
                std::memcpy(&value, token.bytes.data(), sizeof(double));
                out << value;
            }
            else
            {
                out << "real?" << to_hex(token.bytes);
            }
            break;
        case DataType::CHAR:
            if (!token.bytes.empty())
                out << "'" << static_cast<char>(token.bytes[0]) << "'";
            else
                out << "char?";
            break;
        case DataType::BOOL:
            if (!token.bytes.empty())
                out << (token.bytes[0] == 0 ? "false" : "true");
            else
                out << "bool?";
            break;
        case DataType::STRING:
            out << '"' << std::string(token.bytes.begin(), token.bytes.end()) << '"';
            break;
        default:
            out << "unknown:" << to_hex(token.bytes);
            break;
        }

        out << "}";
        return out.str();
    }

    std::string
    row_to_string(const DataRow& row)
    {
        std::ostringstream out;
        out << "row{id=" << row.id << ", flags=" << static_cast<int>(row.flags) << ", tokens=[";

        for (size_t i = 0; i < row.tokens.size(); ++i)
        {
            if (i != 0)
                out << ", ";
            out << token_to_string(row.tokens[i]);
        }

        out << "]}";
        return out.str();
    }

    Args
    parse_args(int argc, char** argv)
    {
        Args args;

        for (int i = 1; i < argc; ++i)
        {
            std::string opt = argv[i];

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

            if (opt == "--page")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--page requires a value");
                args.page_id = argv[++i];
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

        auto schemas = io->read_schemas_meta();
        std::unordered_map<std::string, std::string> schema_name_by_id;
        schema_name_by_id.reserve(schemas.size());
        for (const auto& schema : schemas)
            schema_name_by_id.emplace(schema.id.to_string(), schema.name);

        auto tables = io->read_tables_meta();
        std::unordered_map<std::string, MetaTable> table_by_id;
        table_by_id.reserve(tables.size());
        for (const auto& table : tables)
            table_by_id.emplace(table.id.to_string(), table);

        auto tables_data = io->read_tables_data();

        size_t pages_printed = 0;
        size_t rows_printed = 0;

        for (const auto& [table_id, pages] : tables_data)
        {
            const auto table_it = table_by_id.find(table_id.to_string());
            if (table_it == table_by_id.end())
                continue;

            const auto& table = table_it->second;
            std::string schema_name = "<unknown_schema>";

            const auto schema_it = schema_name_by_id.find(table.schema_id.to_string());
            if (schema_it != schema_name_by_id.end())
                schema_name = schema_it->second;

            if (args.schema_name.has_value() && schema_name != *args.schema_name)
                continue;
            if (args.table_name.has_value() && table.name != *args.table_name)
                continue;

            for (const auto& page : pages)
            {
                if (args.page_id.has_value() && page.id.to_string() != *args.page_id)
                    continue;

                std::cout << "=== PAGE ===\n";
                std::cout << "schema=" << schema_name
                          << " table=" << table.name
                          << " table_id=" << table.id.to_string()
                          << " page_id=" << page.id.to_string() << "\n";
                std::cout << "path=" << page.path.string() << "\n";
                std::cout << "min_rid=" << page.min_rid
                          << " max_rid=" << page.max_rid
                          << " rows_count=" << page.rows_count
                          << " rows_in_vector=" << page.rows.size()
                          << " size=" << page.size
                          << " last_lsn=" << page.last_lsn << "\n";

                for (const auto& row : page.rows)
                {
                    std::cout << "  " << row_to_string(row) << "\n";
                    rows_printed += 1;
                }

                std::cout << "\n";
                pages_printed += 1;
            }
        }

        std::cout << "=== DATA PAGE SUMMARY ===\n";
        std::cout << "pages=" << pages_printed << " rows=" << rows_printed << "\n";

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "dp_dump error: " << ex.what() << "\n";
        return 1;
    }
}
