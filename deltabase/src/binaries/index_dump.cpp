#include "config.hpp"
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
        std::optional<std::string> index_id;
        std::optional<uint64_t> page_id;
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

            if (opt == "--index")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--index requires a value");
                args.index_id = argv[++i];
                continue;
            }

            if (opt == "--page")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--page requires a value");
                args.page_id = std::stoull(argv[++i]);
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
        const auto table_to_indexes = io->map_index_files_for_table();

        std::unordered_map<std::string, std::string> schema_name_by_id;
        schema_name_by_id.reserve(schemas.size());
        for (const auto& schema : schemas)
            schema_name_by_id.emplace(schema.id.to_string(), schema.name);

        std::cout << "=== INDEX DUMP ===\n";
        std::cout << "db=" << db_name << "\n\n";

        size_t index_files_printed = 0;
        size_t pages_printed = 0;
        size_t keys_printed = 0;

        for (const auto& table : tables)
        {
            const auto schema_it = schema_name_by_id.find(table.schema_id.to_string());
            const auto schema_name = schema_it != schema_name_by_id.end() ? schema_it->second
                                                                           : std::string("<unknown_schema>");

            if (args.schema_name.has_value() && schema_name != *args.schema_name)
                continue;

            if (args.table_name.has_value() && table.name != *args.table_name)
                continue;

            const auto table_indexes_it = table_to_indexes.find(table.id);
            if (table_indexes_it == table_to_indexes.end())
                continue;

            std::unordered_map<std::string, MetaIndex> meta_index_by_id;
            meta_index_by_id.reserve(table.indexes.size());
            for (const auto& index : table.indexes)
                meta_index_by_id.emplace(index.id.to_string(), index);

            for (const auto& index_id : table_indexes_it->second)
            {
                if (args.index_id.has_value() && index_id.to_string() != *args.index_id)
                    continue;

                const auto index_file = io->read_index_file(index_id);
                if (!index_file)
                    continue;

                std::string index_name = "<unknown_index>";
                bool is_unique = false;
                const auto meta_idx_it = meta_index_by_id.find(index_id.to_string());
                if (meta_idx_it != meta_index_by_id.end())
                {
                    index_name = meta_idx_it->second.name;
                    is_unique = meta_idx_it->second.is_unique;
                }

                std::cout << "INDEX " << index_name
                          << " id=" << index_id.to_string()
                          << " schema=" << schema_name
                          << " table=" << table.name
                          << " root_page=" << index_file->root_page
                          << " last_page=" << index_file->last_page
                          << " pages=" << index_file->pages.size()
                          << " unique=" << (is_unique ? "true" : "false")
                          << "\n";

                index_files_printed += 1;

                for (const auto& page : index_file->pages)
                {
                    if (args.page_id.has_value() && page.id != *args.page_id)
                        continue;

                    std::cout << "  PAGE id=" << page.id
                              << " parent=" << page.parent
                              << " type=" << (page.is_leaf ? "leaf" : "internal") << "\n";

                    pages_printed += 1;

                    if (page.is_leaf)
                    {
                        const auto& leaf = std::get<LeafIndexNode>(page.data);
                        std::cout << "    keys=" << leaf.keys.size()
                                  << " rows=" << leaf.rows.size()
                                  << " next_leaf=" << leaf.next_leaf << "\n";

                        const auto limit = std::min(leaf.keys.size(), leaf.rows.size());
                        for (size_t i = 0; i < limit; ++i)
                        {
                            std::cout << "      [" << i << "] key=" << token_to_string(leaf.keys[i])
                                      << " row_ptr={page=" << leaf.rows[i].first.to_string()
                                      << ", rid=" << leaf.rows[i].second << "}\n";
                            keys_printed += 1;
                        }
                    }
                    else
                    {
                        const auto& internal = std::get<InternalIndexNode>(page.data);
                        std::cout << "    keys=" << internal.keys.size()
                                  << " children=" << internal.children.size() << "\n";

                        for (size_t i = 0; i < internal.keys.size(); ++i)
                        {
                            std::cout << "      key[" << i << "]=" << token_to_string(internal.keys[i])
                                      << "\n";
                            keys_printed += 1;
                        }

                        for (size_t i = 0; i < internal.children.size(); ++i)
                            std::cout << "      child[" << i << "]=" << internal.children[i] << "\n";
                    }
                }

                std::cout << "\n";
            }
        }

        std::cout << "=== INDEX SUMMARY ===\n";
        std::cout << "files=" << index_files_printed
                  << " pages=" << pages_printed
                  << " keys=" << keys_printed << "\n";

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "index_dump error: " << ex.what() << "\n";
        return 1;
    }
}