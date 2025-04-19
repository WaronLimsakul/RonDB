#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "b-tree.h"

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

Cursor *table_start(Table *table) {
    assert(table);
    // find key = 0 (even if not exist, we still get the most left node)
    Cursor *cursor = table_find(table, 0);

    // don't need to set end_of_table

    return cursor;
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

    void *node = pager_get_page(cursor->table->pager, cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if (cursor->cell_num < num_cells - 1) {
        cursor->cell_num ++;

        // the 0 page_num is default (no next)
    } else if (*leaf_node_next_leaf(node) != 0) {
        cursor->page_num = *leaf_node_next_leaf(node);
        cursor->cell_num = 0;
    } else {
        cursor->end_of_table = true;
    }
}
