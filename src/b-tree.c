#include "b-tree.h"
#include "cursor.h"

#include <assert.h>
#include <stdlib.h>

int32_t* leaf_node_num_cells(void *node) {
    assert(node);
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void *leaf_node_cell(void *node, int32_t cell_num) {
    assert(node);
    return node + LEAF_NODE_HEADER_SIZE + (cell_num * LEAF_NODE_CELL_SIZE);
}

void *leaf_node_key(void *node, int32_t cell_num) {
    assert(node);
    return leaf_node_cell(node, cell_num);
}

void *leaf_node_value(void *node, int32_t cell_num) {
    assert(node);
    return leaf_node_cell(node, cell_num) + LEAF_NODE_VALUE_OFFSET;
}

void init_leaf_node(void *node) {
    *leaf_node_num_cells(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    assert(cursor);
    assert(value);

    void *node = pager_get_page(cursor->page_num);
    unit32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        printf("the page number %d is full\n", cursor->page_num);
        exit(EXIT_FAILURE);
    }

    // if we need to insert in betwee, shift to right to make room
    if (cursor->cell_num < num_cells) {
        for (int i = num_cells; i > cursor->cell_num; i++) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i-1), LEAF_NODE_CELL_SIZE);
        }
    }

    (*leaf_node_num_cells(node))++;
    void *target = leaf_node_cell(node, cursor->cell_num);
    // let's use what I just wrote
    *leaf_node_key(node, cursor->cell_num) = key;
    serialize_row(leaf_node_value(node, cursor->cell_num), value);

    // memcpy(target, &key, LEAF_NODE_KEY_SIZE);
    // memcpy(target + LEAF_NODE_KEY_SIZE, value, LEAF_NODE_VALUE_SIZE);
}
