#include <cstdio>
#include <cstring>
#include <iostream>
#include <ostream>
#include <string.h>

using namespace std;

extern "C" {
    #include "../src/core/include/core.h"
    #include "../src/core/include/data_io.h"
}

namespace test {
    int insert_row() {
        
        return 0;
    }

    int update_row() {

        return 0;
    }

    int scan() {

        return 0;
    }

    MetaColumn *create_mock_column(const char *name, DataType type, DataColumnFlags flags) {
        MetaColumn *col = (MetaColumn *)malloc(sizeof(MetaColumn));
        uuid_generate(col->column_id);
        col->name = strdup(name);
        col->data_type = type;
        col->flags = flags;
        return col;
    }

    MetaTable *create_mock_table() {
        MetaTable *table = (MetaTable *)malloc(sizeof(MetaTable));
        uuid_generate(table->table_id);
        table->name = strdup("users");

        table->columns_count = 4;
        table->columns = (MetaColumn **)malloc(sizeof(MetaColumn *) * table->columns_count);

        // Пример колонок: id (PK, AI), name (NN), email (UN), age (NULLABLE)
        table->columns[0] = create_mock_column("id", DT_INTEGER, (DataColumnFlags)(CF_PK | CF_AI));
        table->columns[1] = create_mock_column("name", DT_STRING, CF_NN);
        table->columns[2] = create_mock_column("email", DT_STRING, CF_UN);
        table->columns[3] = create_mock_column("age", DT_INTEGER, CF_NONE);

        table->has_pk = true;
        memcpy(table->pk, table->columns[0]->column_id, sizeof(uuid_t));

        table->last_rid = 0;
        return table;
    }

    void free_mock_table(MetaTable *table) {
        for (size_t i = 0; i < table->columns_count; ++i) {
            free(table->columns[i]->name);
            free(table->columns[i]);
        }
        free(table->columns);
        free(table->name);
        free(table);
    }

    void print_ph(PageHeader &header) {
        cout << "id: " << header.page_id << endl << "rows: " << header.rows_count << endl << "min_rid: " << header.min_rid << endl << "max_rid: " << header.max_rid << endl;
    }

    void print_uuid(const uuid_t uuid) {
        char uuid_str[37]; // UUID в текстовом виде + нуль-терминатор
        uuid_unparse(uuid, uuid_str);
        std::cout << uuid_str;
    }

    void print_mt(MetaTable *table) {
        std::cout << "Table: " << table->name << "\n";
        std::cout << "Table ID: ";
        print_uuid(table->table_id);
        std::cout << "\n";
        std::cout << "Has PK: " << (table->has_pk ? "Yes" : "No") << "\n";

        if (table->has_pk) {
            std::cout << "Primary Key Column ID: ";
            print_uuid(table->pk);
            std::cout << "\n";
        }

        std::cout << "Last rowid: " << table->last_rid << "\n";

        std::cout << "Columns (" << table->columns_count << "):\n";
        for (size_t i = 0; i < table->columns_count; ++i) {
            MetaColumn *col = table->columns[i];
            std::cout << "  - " << col->name << " (";

            switch (col->data_type) {
                case DT_NULL:   std::cout << "NULL"; break;
                case DT_INTEGER: std::cout << "INTEGER"; break;
                case DT_REAL:    std::cout << "REAL"; break;
                case DT_CHAR:    std::cout << "CHAR"; break;
                case DT_BOOL:    std::cout << "BOOL"; break;
                case DT_STRING:  std::cout << "STRING"; break;
                default:         std::cout << "UNKNOWN"; break;
            }

            std::cout << ")";

            if (col->flags != CF_NONE) {
                std::cout << " [";

                bool first = true;
                if (col->flags & CF_PK)  { std::cout << (first ? "" : ", ") << "PK"; first = false; }
                if (col->flags & CF_FK)  { std::cout << (first ? "" : ", ") << "FK"; first = false; }
                if (col->flags & CF_AI)  { std::cout << (first ? "" : ", ") << "AI"; first = false; }
                if (col->flags & CF_NN)  { std::cout << (first ? "" : ", ") << "NN"; first = false; }
                if (col->flags & CF_UN)  { std::cout << (first ? "" : ", ") << "UN"; first = false; }

                std::cout << "] ";
            }

            std::cout << "| Id: ";
            print_uuid(col->column_id);

            std::cout << "\n";
        }
    }

    DataRow* create_mock_row() {
        DataRow* row = (DataRow*)malloc(sizeof(DataRow));
        if (!row) return nullptr;

        row->flags = RF_NONE;
        row->count = 4;
        row->tokens = (DataToken**)malloc(row->count * sizeof(DataToken*));
        if (!row->tokens) {
            free(row);
            return nullptr;
        }

        // Token 0: INTEGER
        row->tokens[0] = (DataToken*)malloc(sizeof(DataToken));
        row->tokens[0]->type = DT_INTEGER;
        row->tokens[0]->size = sizeof(int32_t);
        row->tokens[0]->bytes = (char*)malloc(sizeof(int32_t));
        int32_t val0 = 2;
        memcpy(row->tokens[0]->bytes, &val0, sizeof(int32_t));

        // Token 1: STRING
        const char* str = "Bangladesh";
        size_t str_len = strlen(str);
        row->tokens[1] = (DataToken*)malloc(sizeof(DataToken));
        row->tokens[1]->type = DT_STRING;
        row->tokens[1]->size = str_len;
        row->tokens[1]->bytes = (char*)malloc(str_len);
        memcpy(row->tokens[1]->bytes, str, str_len);

        const char* str2 = "Kuba";
        size_t str_len2 = strlen(str2);
        row->tokens[2] = (DataToken*)malloc(sizeof(DataToken));
        row->tokens[2]->type = DT_STRING;
        row->tokens[2]->size = str_len2;
        row->tokens[2]->bytes = (char*)malloc(str_len2);
        memcpy(row->tokens[2]->bytes, str2, str_len2);

        row->tokens[3] = (DataToken*)malloc(sizeof(DataToken));
        row->tokens[3]->type = DT_INTEGER;
        row->tokens[3]->size = sizeof(int32_t);
        row->tokens[3]->bytes = (char*)malloc(sizeof(int32_t));
        int32_t val1 = 16;
        memcpy(row->tokens[3]->bytes, &val1, sizeof(int32_t));

        return row;
    }

    string get_datatype_str(DataType type) {
        switch (type) {
            case DT_INTEGER: return "INTEGER";
            case DT_STRING: return "STRING";
            case DT_NULL: return "NULL";
            case DT_CHAR: return "CHAR";
            case DT_REAL: return "REAL";
            case DT_BOOL: return "BOOL";
        }
        return "";
    }

    void print_dr(DataRow &row) {
        cout << "\nId: " << row.row_id << endl;
        cout << "Flags: " << row.flags << endl;
        cout << "Tokens:";

        for (size_t i = 0; i < row.count; ++i) {
            DataToken *token = row.tokens[i];
            cout << "[ " << get_datatype_str(token->type) << ": ";

            switch (token->type) {
                case DT_INTEGER: {
                                     int value;
                                     memcpy(&value, token->bytes, sizeof(int));
                                     cout << value;
                                     break;
                                 }
                case DT_STRING: {
                                    string value(token->bytes, token->size);
                                    cout << '"' << value << '"';
                                    break;
                                }
                case DT_REAL: {
                                  double value;
                                  memcpy(&value, token->bytes, sizeof(double));
                                  cout << value;
                                  break;
                              }
                case DT_BOOL: {
                                  bool value;
                                  memcpy(&value, token->bytes, sizeof(bool));
                                  cout << (value ? "true" : "false");
                                  break;
                              }
                case DT_CHAR: {
                                  char value;
                                  memcpy(&value, token->bytes, sizeof(char));
                                  cout << '\'' << value << '\'';
                                  break;
                              }
                case DT_NULL: {
                                  cout << "NULL";
                                  break;
                              }
                default: {
                             cout << "Unknown";
                             break;
                         }
            }

            cout << " ], ";
        }

        cout << endl;
    }

    void print_dt(DataTable& table) {
        cout << "Rows count: " << table.rows_count << endl;;
        cout << "Rows: " << endl;
        for (size_t i = 0; i < table.rows_count; i++) {
            cout << '[' << endl;
            print_dr(*table.rows[i]);
            cout << endl << ']' << endl;
        }
    }

    DataFilter *create_mock_filter() {
        uuid_t id_col;
        uuid_parse("16f92ad1-d56f-4888-90c3-a92979aa424c", id_col);
        uuid_t name_col;
        uuid_parse("16f92ad1-d56f-4888-90c3-a92979aa424c", name_col);

        int *id_value = (int *)malloc(sizeof(int));
        *id_value = 2;
        // Итоговый логический фильтр: id = 1 AND name = "hello"
        DataFilter *root_filter = (DataFilter *)malloc(sizeof(DataFilter));
        root_filter->is_node = false;
        root_filter->data.condition = (DataFilterCondition){
            .op = OP_EQ,
            .type = DT_INTEGER,
            .value = id_value,
        };
        memcpy(&root_filter->data.condition.column_id, id_col, sizeof(uuid_t));
        return root_filter;

        // // Значения
        // const char *name_value = "Bangladesh";
        // size_t name_len = strlen(name_value);
        //
        // // Фильтр для: id = 1
        // DataFilter *filter_id = (DataFilter *)malloc(sizeof(DataFilter));
        // filter_id->is_node = false;
        // filter_id->data.condition = (DataFilterCondition){
        //     .op = OP_EQ,
        //     .type = DT_INTEGER,
        //     .value = &id_value,
        // };
        // memcpy(&filter_id->data.condition.column_id, id_col, sizeof(uuid_t));
        //
        // // Фильтр для: name = "hello"
        // DataFilter *filter_name = (DataFilter *)malloc(sizeof(DataFilter));
        // filter_name->is_node = false;
        // filter_name->data.condition = (DataFilterCondition){
        //     .op = OP_EQ,
        //     .type = DT_STRING,
        //     .value = (void *)name_value,
        // };
        // memcpy(&filter_name->data.condition.column_id, name_col, sizeof(uuid_t));
        //
        // // Итоговый логический фильтр: id = 1 AND name = "hello"
        // DataFilter *root_filter = (DataFilter *)malloc(sizeof(DataFilter));
        // root_filter->is_node = true;
        // root_filter->data.node = (DataFilterNode){
        //     .left = filter_id,
        //     .op = LOGIC_AND,
        //     .right = filter_name
        // };
        // return root_filter;
    }

    DataRowUpdate *create_mock_row_update() { 
        DataRowUpdate *update = new DataRowUpdate;
        update->column_indices = (uuid_t*)malloc(sizeof(uuid_t*));
        update->count = 1;
        uuid_parse("7adad1b1-6a51-4a68-8b69-d435ff4cf55d", update->column_indices[0]); // email column
        update->values = (void **)malloc(sizeof (void*));
        char *value = (char *)malloc(15);
        strcpy(value, "hello");

        update->values[0] = value;
        return update;
    }
}

using namespace test;

int main() {
    // cout << create_database("testdb") << endl << create_table("testdb", create_mock_table()) << endl;
    int res = 0;
    MetaTable metatable;
    if ((res = get_table_schema("testdb", "users", &metatable)) != 0) {
        cout << res << endl;
        return 1;
    }
    test::print_mt(&metatable);

    // cout << insert_row("testdb", "users", create_mock_row()) << endl;
    //
    // cout << update_row_by_filter("testdb", "users", create_mock_filter(), create_mock_row_update()) << endl;
    //
    cout << delete_row_by_filter("testdb", "users", create_mock_filter()) << endl;

    DataTable table;
    res = full_scan("testdb", "users", &table);
    if (res != 0) {
        cout << res << endl;
        return 1;
    }
    test::print_dt(table);
    /*
    
    */
    // update_row_by_filter("testdb", "users", );
}
