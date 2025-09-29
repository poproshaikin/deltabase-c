#include "include/objects/meta_object.hpp"

#include "../converter/include/converter.hpp"
#include "../misc/include/utils.hpp"

namespace storage {
    meta_column::meta_column() : id(make_uuid_str()) {
    }

    meta_column::meta_column(const sql::ColumnDefinition& def)
        : meta_column(
              def.name.value,
              converter::convert_kw_to_vt(def.type.get_detail<sql::SqlKeyword>()),
              converter::convert_tokens_to_cfs(def.constraints)
          ) {
    }

    meta_column::meta_column(const std::string& name, ValueType type, meta_column_flags flags) 
        : name(name), type(type), flags(flags) {
    }
}
