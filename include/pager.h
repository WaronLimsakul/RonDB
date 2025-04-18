#pragma once

#include <stdint.h>
#include "config.h"

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void *pages[TABLE_MAX_PAGES];
} Pager;

Pager *pager_open(char *file_name);

void *pager_get_page(Pager *pager, int page_num);

void pager_flush(Pager* pager, int page_num);

int pager_get_unused_page(Pager *pager);
