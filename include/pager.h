#pragma once

#include <stdint.h>
#include "config.h"

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

Pager *open_pager(char *file_name);

void *pager_get_page(Pager *pager, int page_num);

void pager_flush(Pager* pager, int page_num, size_t size);
