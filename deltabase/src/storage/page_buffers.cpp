#include "include/pages/page_buffers.hpp"
#include "cache/accessors.hpp"
#include "file_manager.hpp"

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
        for (const auto& page : pages_)
        {
            if (page.is_dirty)
            {

            }
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