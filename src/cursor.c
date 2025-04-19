#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "b-tree.h"

Cursor *table_start(Table *table) {
    assert(table);

    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void *root_node = pager_get_page(table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node); // but root is not leaf no?
    cursor->end_of_table = (num_cells == 0);

    return cursor;
}

Cursor *table_find(Table *table, uint32_t key) {
    assert(table);

    void *root_node = pager_get_page(table->pager, table->root_page_num);
    switch (node_type(root_node)) {
        case LEAF_NODE:
            return leaf_node_find(table, table->root_page_num, key);
        case INTERNAL_NODE:
            return internal_node_find(table, table->root_page_num, key);
    }
    return NULL;
}

// get pointer to a cell cursor point to.
void *cursor_value(Cursor *cursor) {
    assert(cursor);

    void *page = pager_get_page(cursor->table->pager, cursor->page_num);
    return leaf_node_value(page, cursor->cell_num);
}

// just advance in the root node for now.
void cursor_advance(Cursor *cursor) {
    assert(cursor);
    assert(cursor->table);
    assert(cursor->cell_num <= LEAF_NODE_MAX_CELLS); // can be at the end but not more

    void *root_node = pager_get_page(cursor->table->pager, cursor->table->root_page_num);

    uint32_t num_cells = *leaf_node_num_cells(root_node);
    if (cursor->cell_num < num_cells) {
        cursor->cell_num ++;
    }

    if (cursor->cell_num == num_cells) {
        cursor->end_of_table = true;
    }
}
