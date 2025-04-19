#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>

#include "table.h"
#include "b-tree.h"

const uint32_t ID_SIZE = size_of_attr(Row, id);
const uint32_t NAME_SIZE = size_of_attr(Row, name);
const uint32_t EMAIL_SIZE = size_of_attr(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t NAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = NAME_OFFSET + NAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + NAME_SIZE + EMAIL_SIZE;

Table *db_open(char *file_name) {

    Pager *pager = pager_open(file_name);
    Table *table = malloc(sizeof(Table));

    table->pager = pager;

    table->root_page_num = 0;
    // init empty leaf node if the file is empty
    if (pager->num_pages == 0) {
        void *node = pager_get_page(table->pager, 0);
        init_leaf_node(node);
        node_set_is_root(node, true);
    }
    return table;
}

void db_close(Table *table) {
    assert(table);

    Pager *pager = table->pager;

    for (int i = 0; i < table->pager->num_pages; i++) {
        pager_flush(pager, i);
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
