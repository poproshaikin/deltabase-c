#include "static_storage.hpp"
#include "wal_log.hpp"
#include "wal_manager_factory.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using namespace types;

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

    std::string
    schema_to_string(const MetaSchema& schema)
    {
        std::ostringstream out;
        out << "schema{name='" << schema.name << "', id=" << schema.id.to_string()
            << ", db='" << schema.db_name << "'}";
        return out.str();
    }

    std::string
    table_to_string(const MetaTable& table)
    {
        std::ostringstream out;
        out << "table{name='" << table.name << "', id=" << table.id.to_string()
            << ", schema_id=" << table.schema_id.to_string() << ", last_rid=" << table.last_rid
            << ", columns=" << table.columns.size() << "}";
        return out.str();
    }

    std::string
    type_to_string(WALRecordType type)
    {
        switch (type)
        {
        case WALRecordType::INSERT:
            return "INSERT";
        case WALRecordType::UPDATE:
            return "UPDATE";
        case WALRecordType::DELETE:
            return "DELETE";
        case WALRecordType::CREATE_TABLE:
            return "CREATE_TABLE";
        case WALRecordType::UPDATE_TABLE:
            return "UPDATE_TABLE";
        case WALRecordType::DELETE_TABLE:
            return "DELETE_TABLE";
        case WALRecordType::CREATE_SCHEMA:
            return "CREATE_SCHEMA";
        case WALRecordType::UPDATE_SCHEMA:
            return "UPDATE_SCHEMA";
        case WALRecordType::DELETE_SCHEMA:
            return "DELETE_SCHEMA";
        case WALRecordType::BEGIN_TXN:
            return "BEGIN_TXN";
        case WALRecordType::COMMIT_TXN:
            return "COMMIT_TXN";
        case WALRecordType::ROLLBACK_TXN:
            return "ROLLBACK_TXN";
        case WALRecordType::CLR_INSERT:
            return "CLR_INSERT";
        case WALRecordType::CLR_UPDATE:
            return "CLR_UPDATE";
        case WALRecordType::CLR_DELETE:
            return "CLR_DELETE";
        default:
            return "UNKNOWN";
        }
    }

    struct Args
    {
        std::string db_name;
        std::optional<LSN> from_lsn;
        std::optional<LSN> to_lsn;
    };

    Args
    parse_args(int argc, char** argv)
    {
        if (argc < 2)
            throw std::runtime_error(
                "Usage: wal_dump.exe <db_name> [--from <lsn>] [--to <lsn>]"
            );

        Args args;
        args.db_name = argv[1];

        for (int i = 2; i < argc; ++i)
        {
            std::string opt = argv[i];

            if (opt == "--from")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--from requires a value");
                args.from_lsn = static_cast<LSN>(std::stoull(argv[++i]));
                continue;
            }
            if (opt == "--to")
            {
                if (i + 1 >= argc)
                    throw std::runtime_error("--to requires a value");
                args.to_lsn = static_cast<LSN>(std::stoull(argv[++i]));
                continue;
            }

            throw std::runtime_error("Unknown argument: " + opt);
        }

        return args;
    }
}

int
main(int argc, char** argv)
{
    try
    {
        auto args = parse_args(argc, argv);

        misc::StaticStorage::set_executable_path(std::filesystem::absolute(argv[0]).parent_path());

        auto cfg = types::Config::std(args.db_name);
        wal::WalManagerFactory factory;
        auto wal_manager = factory.make(cfg);

        auto records = wal_manager->read_all_logs();
        std::sort(
            records.begin(),
            records.end(),
            [](const types::WALRecord& lhs, const types::WALRecord& rhs)
            {
                return std::visit([](const auto& r) { return r.lsn; }, lhs)
                    < std::visit([](const auto& r) { return r.lsn; }, rhs);
            }
        );

        std::map<std::string, size_t> counts;

        for (const auto& record : records)
        {
            bool in_range = std::visit(
                [&](const auto& r)
                {
                    if (args.from_lsn.has_value() && r.lsn < *args.from_lsn)
                        return false;
                    if (args.to_lsn.has_value() && r.lsn > *args.to_lsn)
                        return false;
                    return true;
                },
                record
            );
            if (!in_range)
                continue;

            std::visit(
                [&](const auto& r)
                {
                    const std::string type = type_to_string(r.type);
                    counts[type] += 1;

                    std::cout << "[" << r.lsn << "] " << type
                              << " txn=" << r.txn_id.to_string()
                              << " prev=" << r.prev_lsn;

                    using R = std::decay_t<decltype(r)>;
                    if constexpr (std::is_same_v<R, InsertRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " after=" << row_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, UpdateRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " before=" << row_to_string(r.before)
                                  << " after=" << row_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, DeleteRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " before=" << row_to_string(r.before);
                    }
                    else if constexpr (std::is_same_v<R, CLRInsertRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " undo_next=" << r.undo_next_lsn
                                  << " after=" << row_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, CLRUpdateRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " undo_next=" << r.undo_next_lsn
                                  << " before=" << row_to_string(r.before)
                                  << " after=" << row_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, CLRDeleteRecord>)
                    {
                        std::cout << " table=" << r.table_id.to_string()
                                  << " page=" << r.page_id.to_string()
                                  << " undo_next=" << r.undo_next_lsn
                                  << " before=" << row_to_string(r.before);
                    }
                    else if constexpr (std::is_same_v<R, CreateSchemaRecord>)
                    {
                        std::cout << " after=" << schema_to_string(r.schema);
                    }
                    else if constexpr (std::is_same_v<R, UpdateSchemaRecord>)
                    {
                        std::cout << " before=" << schema_to_string(r.before)
                                  << " after=" << schema_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, DeleteSchemaRecord>)
                    {
                        std::cout << " before=" << schema_to_string(r.before);
                    }
                    else if constexpr (std::is_same_v<R, CreateTableRecord>)
                    {
                        std::cout << " after=" << table_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, UpdateTableRecord>)
                    {
                        std::cout << " before=" << table_to_string(r.before)
                                  << " after=" << table_to_string(r.after);
                    }
                    else if constexpr (std::is_same_v<R, DeleteTableRecord>)
                    {
                        std::cout << " before=" << table_to_string(r.before);
                    }

                    std::cout << "\n";
                },
                record
            );
        }

        std::cout << "\n=== WAL SUMMARY ===\n";
        for (const auto& [type, count] : counts)
            std::cout << type << ": " << count << "\n";

        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "wal_dump error: " << ex.what() << "\n";
        return 1;
    }
}
