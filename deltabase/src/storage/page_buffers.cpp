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
        DataPage& page = has_available_page(size) ? get_available_page(size) : create_page(table);

        page.insert_row(table, row);
        page.mark_dirty();
        tables_.mark_dirty(table.id);

        return 0;
    }

    void
    PageBuffers::flush()
    {
        struct PageFlushInfo
        {
            std::string page_id;
            std::string table_id;
            const DataPage &page;
            PageFlushInfo(std::string page_id, std::string table_id, const DataPage& page)
                : page_id(page_id), table_id(table_id), page(page)
            {
            }
        };

        std::vector<PageFlushInfo> pages_to_flush;
        
        // Iterate over all pages and collect dirty ones
        for (auto& entry : pages_)
        {
            if (entry.is_dirty)
            {
                auto& page = entry.value;
                
                std::string table_id = page.table_id();
                
                if (table_id.empty()) 
                    continue;
                
                pages_to_flush.emplace_back(page.id(), table_id, page);
            }
        }
        
        for (const auto& flush_info : pages_to_flush)
        {
            if (!tables_.has(flush_info.table_id)) {
                throw std::runtime_error(
                    std::format("Cannot flush page {}: table {} not found in cache",
                               flush_info.page_id, flush_info.table_id)
                );
            }
            
            auto& table = tables_.get(flush_info.table_id);
            
            if (!schemas_.has(table.schema_id)) {
                throw std::runtime_error(
                    std::format("Cannot flush page {}: schema {} not found in cache",
                               flush_info.page_id, table.schema_id)
                );
            }
            
            auto& schema = schemas_.get(table.schema_id);

            fm_.save_page(db_name_, schema.name, table.name, flush_info.page);
            pages_.mark_clean(flush_info.page_id);
        }
    }

    DataPage&
    PageBuffers::create_page(MetaTable& table)
    {
        auto& schema = schemas_.get(table.schema_id);

        DataPage page = fm_.create_page(db_name_, schema.name, table.id, table.name);
        pages_.put(std::move(page));

        return pages_.get(make_key(page));
    }

    bool
    PageBuffers::has_available_page(uint64_t payload_size) noexcept
    {
        for (const auto& page : pages_)
            if (page.value.size_ + payload_size < DataPage::max_size)
                return true;

        return false;
    }

    DataPage&
    PageBuffers::get_available_page(uint64_t payload_size)
    {
        for (auto& page : pages_)
            if (page.value.size_ + payload_size < DataPage::max_size)
                return page.value;

        throw std::runtime_error("PageBuffers::get_available_page: could not find an available page");
    }

    void
    PageBuffers::update_page(DataPage& page) 
    {
        std::string table_id = page.table_id();
        if (table_id.empty())
            throw std::runtime_error(
                std::format("Cannot update page {}: page has no table_id", page.id())
            );

        if (!tables_.has(table_id))
            throw std::runtime_error(
                std::format(
                    "Cannot update page {}: table {} not found in cache", page.id(), table_id
                )
            );
        auto& table = tables_.get(table_id);

        if (!schemas_.has(table.schema_id))
            throw std::runtime_error(
                std::format(
                    "Cannot update page {}: schema {} not found in cache",
                    page.id(),
                    table.schema_id
                )
            );
        auto& schema = schemas_.get(table.schema_id);
        
        fm_.save_page(db_name_, schema.name, table.name, page);
        pages_.mark_clean(page.id());
    }
} // namespace storage