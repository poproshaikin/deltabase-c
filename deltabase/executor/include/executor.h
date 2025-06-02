#include <memory>
#include "../../core/include/table.h"

namespace executor {
    class QueryExecutor {
        public:
            std::unique_ptr<DataTable> execute_select(DataScheme scheme); 
    };
}
