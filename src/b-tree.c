#include "b-tree.h"
#include "cursor.h"
#include "vm.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// common node header consts
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t); // dedicate 1 byte for that. easy but wasty
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t); // again wasty but easy
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE + NODE_TYPE_OFFSET;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_SIZE + IS_ROOT_OFFSET;
const uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// additional leaf node header
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

// leaf node body const
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;

// Can't use ROW_SIZE because it's in table.c and we need to know in compile time
const uint32_t LEAF_NODE_VALUE_SIZE =
    size_of_attr(Row, id) + size_of_attr(Row, name) + size_of_attr(Row, email);
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;

const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

// only check the first 8 bits
NodeType node_type(void *node) {
    uint8_t value = *(uint8_t*)(node + NODE_TYPE_OFFSET);
    return (NodeType)value;
}

// only change the first 8 bits
void set_node_type(void *node, NodeType type) {
    *(uint8_t*)(node + NODE_TYPE_OFFSET) = (uint8_t)type;
}

int32_t* leaf_node_num_cells(void *node) {
    assert(node);
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void *leaf_node_cell(void *node, int32_t cell_num) {
    assert(node);
    return node + LEAF_NODE_HEADER_SIZE + (cell_num * LEAF_NODE_CELL_SIZE);
}

uint32_t *leaf_node_key(void *node, int32_t cell_num) {
    assert(node);
    return leaf_node_cell(node, cell_num);
}

void *leaf_node_value(void *node, int32_t cell_num) {
    assert(node);
    return leaf_node_cell(node, cell_num) + LEAF_NODE_VALUE_OFFSET;
}

void init_leaf_node(void *node) {
    set_node_type(node, LEAF_NODE);
    *leaf_node_num_cells(node) = 0;
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    assert(cursor);
    assert(value);

    void *node = pager_get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        printf("the page number %d is full\n", cursor->page_num);
        exit(EXIT_FAILURE);
    }

    // if we need to insert in betwee, shift to right to make room
    if (cursor->cell_num < num_cells) {
        for (int i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node, i), leaf_node_cell(node, i-1), LEAF_NODE_CELL_SIZE);
        }
    }

    (*leaf_node_num_cells(node))++;
    // let's use what I just wrote
    *leaf_node_key(node, cursor->cell_num) = key;
    serialize_row(leaf_node_value(node, cursor->cell_num), value);

    // alt:
    // void *target = leaf_node_cell(node, cursor->cell_num);
    // memcpy(target, &key, LEAF_NODE_KEY_SIZE);
    // memcpy(target + LEAF_NODE_KEY_SIZE, value, LEAF_NODE_VALUE_SIZE);
}

static bool high_key(void *node, uint32_t key, uint32_t target) {
    return *leaf_node_key(node, key) >= target;
}

Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key) {
    assert(table);

    void *node = pager_get_page(table->pager, page_num);
    uint32_t low = 0;

    uint32_t num_cells = *leaf_node_num_cells(node);
    uint32_t high = num_cells;

    while (low < high) {
        uint32_t mid = low + ((high- low) / 2);
        if (high_key(node, mid, key)) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    // now low is what I want
    Cursor *cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->cell_num = low;
    cursor->page_num = page_num;
    cursor->end_of_table = low == num_cells;

    return cursor;
}
