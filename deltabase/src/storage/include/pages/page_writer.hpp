#pragma once

#include "pages/page.hpp"
#include "../../misc/include/utils.hpp"

namespace storage 
{
    // прочитать заголовок
    DataPageHeader
    read_page_header(MemoryStream& stream);
}