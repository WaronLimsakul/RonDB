#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "table.h"

typedef struct {
    Table *table;
    uint32 page_num;
    uint32 cell_num;
    bool end_of_table;
} Cursor;

Cursor *table_start(Table *table);
Cursor *table_end(Table *table);
void *cursor_value(Cursor *cursor);
void cursor_advance(Cursor *cursor);
