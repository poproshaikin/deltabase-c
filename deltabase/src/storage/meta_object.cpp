#include "include/objects/meta_object.hpp"

#include "../converter/include/converter.hpp"
#include "../misc/include/utils.hpp"

namespace storage {
    MetaColumn::MetaColumn() : id(make_uuid_str()) {
    }

    MetaColumn::MetaColumn(const sql::ColumnDefinition& def)
        : MetaColumn(
              def.name.value,
              converter::convert_kw_to_vt(def.type.get_detail<sql::SqlKeyword>()),
              converter::convert_tokens_to_cfs(def.constraints)
          ) {
    }

    MetaColumn::MetaColumn(const std::string& name, ValueType type, meta_column_flags flags) 
        : name(name), type(type), flags(flags) {
    }
}
