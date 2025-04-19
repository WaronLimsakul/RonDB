#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "table.h"
#include "config.h"

// cursor should be used to poitn to leaf node only

typedef struct {
    Table *table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
} Cursor;

Cursor *table_start(Table *table);
Cursor *table_find(Table *table, uint32_t key);
void *cursor_value(Cursor *cursor);
void cursor_advance(Cursor *cursor);
