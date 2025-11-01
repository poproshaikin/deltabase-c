#include "include/pages/page_buffers.hpp"
#include "cache/accessors.hpp"
#include "file_manager.hpp"
#include <format>
#include <stdexcept>

namespace storage
{
    PageBuffers::PageBuffers(
        const std::string& db_name,
        FileManager& fm,
        EntityCache<std::string, MetaSchema, MetaSchemaAccessor, make_key>& schemas,
        EntityCache<std::string, MetaTable, MetaTableAccessor, make_key>& tables
    )
        : db_name_(db_name), fm_(fm), schemas_(schemas), tables_(tables), pages_(DataPageAccessor(db_name, fm))
    {
    }

    void
    PageBuffers::load()
    {
        // проехаться по всем папкам в папке базы данных, 
        // найти страницы и для каждой:
        // спарсить и сохранить в обьекты DataPage
        // надо сделать набор функций парсеров данных, по сути то ядро которое было раньше, 
        // только работать с потоком байтов, а не файлом, а так все по сути также

        auto pages = fm_.load_all_pages(db_name_);
        pages_.init_with(std::move(pages));
    }

    int
    PageBuffers::insert_row(MetaTable& table, DataRow& row)
    {
        uint64_t size = row.estimate_size();
        DataPage& page = has_available_page(size) ? get_available_page(size) : create_page();

        page.insert_row(table, row);
        page.mark_dirty();
        tables_.mark_dirty(table.id);

        return 0;
    }

    void
    PageBuffers::flush()
    {
        // Collect all dirty pages and their metadata in a snapshot
        // to minimize time holding the lock
        struct PageFlushInfo {
            std::string page_id;
            std::string table_id;
            bytes_v serialized_data;
        };
        
        std::vector<PageFlushInfo> pages_to_flush;
        
        // Iterate over all pages and collect dirty ones
        for (auto& entry : pages_)
        {
            if (entry.is_dirty)
            {
                const auto& page = entry.value;
                
                // Need to resolve schema_name and table_name from table_id
                std::string table_id = page.table_id();
                
                if (table_id.empty()) {
                    // Skip pages without table_id (shouldn't happen, but be safe)
                    continue;
                }
                
                // Serialize the page while we have access to it
                PageFlushInfo info;
                info.page_id = page.id();
                info.table_id = table_id;
                info.serialized_data = page.serialize();
                
                pages_to_flush.push_back(std::move(info));
            }
        }
        
        // Now flush each page to disk
        for (const auto& flush_info : pages_to_flush)
        {
            // Get table to find schema_name and table_name
            if (!tables_.has(flush_info.table_id)) {
                // Table not in cache, this is an error
                throw std::runtime_error(
                    std::format("Cannot flush page {}: table {} not found in cache",
                               flush_info.page_id, flush_info.table_id)
                );
            }
            
            auto& table = tables_.get(flush_info.table_id);
            
            // Get schema to find schema_name
            if (!schemas_.has(table.schema_id)) {
                // Schema not in cache, this is an error
                throw std::runtime_error(
                    std::format("Cannot flush page {}: schema {} not found in cache",
                               flush_info.page_id, table.schema_id)
                );
            }
            
            auto& schema = schemas_.get(table.schema_id);
            
            // Write the page to disk using FileManager
            // Note: save_page expects a DataPage, but we only have serialized data
            // We need to deserialize it back or change the approach
            
            // Better approach: Let's reconstruct the page temporarily
            DataPage temp_page;
            if (!DataPage::try_deserialize(flush_info.serialized_data, temp_page)) {
                throw std::runtime_error(
                    std::format("Failed to deserialize page {} for flushing", flush_info.page_id)
                );
            }
            
            fm_.save_page(db_name_, schema.name, table.name, temp_page);
            
            // Mark the page as clean in the cache
            pages_.mark_clean(flush_info.page_id);
        }
    }

    void
    PageBuffers::update_page(DataPage& page) 
    {
        // найти старый файл
        // переименовать страницу
        // старый файл удалить
        // создать новый файл с новым именем 
        // записать туда новый контент


    }
} // namespace storage