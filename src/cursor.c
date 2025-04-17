#include <stdlib.h>
#include <assert.h>
#include "cursor.h"
#include "config.h"

Cursor *table_start(Table *table) {
    assert(table);

    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = 0;
    cursor->end_of_table = (table->num_rows == 0);

    return cursor;
}

Cursor *table_end(Table *table) {
    assert(table);

    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->row_num = table->num_rows;
    cursor->end_of_table = true;

    return cursor;
}

// get pointer to a row cursor point to.
void *cursor_value(Cursor *cursor) {
    assert(cursor);

    uint32_t page_num = cursor->row_num / ROWS_PER_PAGE;
    void *page = pager_get_page(cursor->table->pager, page_num);

    uint32_t rows_offset = cursor->row_num % ROWS_PER_PAGE;
    uint32_t bytes_offset = rows_offset * ROW_SIZE;
    return page + bytes_offset;
}

void cursor_advance(Cursor *cursor) {
    assert(cursor);
    assert(cursor->table);
    assert(cursor->row_num <= cursor->table->num_rows); // can be at the end but not more

    if (cursor->row_num < cursor->table->num_rows) {
        cursor->row_num ++;
    }

    if (cursor->row_num == cursor->table->num_rows) {
        cursor->end_of_table = true;
    }
}
