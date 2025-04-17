#include <stdlib.h>
#include <assert.h>
#include "cursor.h"
#include "config.h"
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

Cursor *table_end(Table *table) {
    assert(table);

    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;

    // IDK why we use the root page. I thought we need last page in the table
    cursor->page_num = table->root_page_num;

    void *last_page = pager_get_page(table->pager, cursor->page_num);
    cursor->cell_num = leaf_node_num_cells(last_page);
    cursor->end_of_table = true;

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

    void *root_node = pager_get_page(cursor->table->pager, cursor->table->root_page_num);

    uint32_t num_cells = *leaf_node_num_cells(root_node);
    if (cursor->cell_num < num_cells) {
        cursor->cell_num ++;
    }

    if (cursor->cell_num == num_cells) {
        cursor->end_of_table = true;
    }
}
