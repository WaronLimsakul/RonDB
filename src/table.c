#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "table.h"

const uint32_t ID_SIZE = size_of_attr(Row, id);
const uint32_t NAME_SIZE = size_of_attr(Row, name);
const uint32_t EMAIL_SIZE = size_of_attr(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t NAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = NAME_OFFSET + NAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + NAME_SIZE + EMAIL_SIZE;

const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

Table *db_open(char *file_name) {

    Pager *pager = open_pager(file_name);
    Table *table = malloc(sizeof(Table));

    table->num_rows = pager->file_length / ROW_SIZE;
    table->pager = pager;
    return table;
}

void db_close(Table *table) {
    assert(table);

    Pager *pager = table->pager;
    // deal with full pages first
    int num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (int i = 0; i < num_full_pages; i++) {
        pager_flush(pager, i, PAGE_SIZE);
    }
    // deal with partial page
    int partial_page_rows = table->num_rows % ROWS_PER_PAGE;
    if (partial_page_rows > 0) {
        pager_flush(pager, num_full_pages, ROW_SIZE * partial_page_rows);
    }

    int close_status = close(pager->file_descriptor);
    if (close_status == -1) {
        printf("error closing db connection\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        if (pager->pages[i] != NULL) {
            free(pager->pages[i]);
        }
    }
    free(pager);
    free(table);
}


// get row address from table and row number
// malloc a new page if required
void *table_get_row(Table *table, uint32_t row_nums) {
    assert(table);
    assert(row_nums < TABLE_MAX_ROWS);

    int page_num = row_nums / ROWS_PER_PAGE;
    void *page_addr = pager_get_page(table->pager, page_num);

    int row_num_in_page = row_nums % ROWS_PER_PAGE;
    int bytes_offset = row_num_in_page * ROW_SIZE;
    return page_addr + bytes_offset;
}
