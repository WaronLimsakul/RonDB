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

// want the right to be biasly less
const uint32_t LEAF_NODE_SPLIT_RIGHT_CELLS = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_SPLIT_LEFT_CELLS =
    LEAF_NODE_MAX_CELLS + 1 - LEAF_NODE_SPLIT_RIGHT_CELLS;

const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_NUM_KEYS_OFFSET;
const uint32_t INTERNAL_NODE_HEADER_SIZE = (
    COMMON_NODE_HEADER_SIZE +
    INTERNAL_NODE_NUM_KEYS_SIZE +
    INTERNAL_NODE_RIGHT_CHILD_SIZE
);

const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;


// only check the first 8 bits
NodeType node_type(void *node) {
    uint8_t value = *(uint8_t*)(node + NODE_TYPE_OFFSET);
    return (NodeType)value;
}

bool node_is_root(void *node) {
    return (bool)(node + IS_ROOT_OFFSET);
}

void node_set_is_root(void *node, bool is_root) {
    *(bool *)(node + IS_ROOT_OFFSET) = is_root;
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

uint32_t *internal_node_num_keys(void *node) {
    assert(node);
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

void *internal_node_right_child(void *node) {
    assert(node);
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

void *internal_node_cell(void *node, uint32_t cell_num) {
    assert(node);
    assert(cell_num < *internal_node_num_keys(node));

    return node + INTERNAL_NODE_HEADER_SIZE + (INTERNAL_NODE_CELL_SIZE * cell_num);
}

// child is at the front of the cell;
void *internal_node_child(void *node, uint32_t cell_num) {
    return internal_node_cell(node, cell_num);
}

void *internal_node_key(void *node, uint32_t cell_num) {
    return internal_node_cell(node, cell_num) + INTERNAL_NODE_CHILD_SIZE;
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

// leaf node must be full first to call this.
// and why do we need the key?
/*
plan to split
1. treat the node as inserted node
2. for loop from the bottom
3. properly assign the destination
4. have if-else statement to check target index at the end
    - if current = target: write value
    - if current > target: write old[i-1] because imagine it's already shifted
    - if current < target: write old[i] because it's not shifted
*/
void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value) {

    Pager *pager = cursor->table->pager;
    void *old_node = pager_get_page(pager, cursor->page_num);

    int new_page_num = pager_get_unused_page(pager);
    void *new_node = pager_get_page(pager, new_page_num);
    init_leaf_node(new_node);

    uint32_t target_idx = cursor->cell_num;
    for (int cur = LEAF_NODE_MAX_CELLS; cur > -1; cur--) {
        void *dest_node;
        if (cur < LEAF_NODE_SPLIT_LEFT_CELLS) {
            dest_node = old_node;
        } else {
            dest_node = new_node;
        }

        int cur_at_dest = cur % LEAF_NODE_SPLIT_LEFT_CELLS;
        void *destination = leaf_node_cell(dest_node, cur_at_dest);

        if (cur == target_idx) {
            *leaf_node_key(dest_node, cur_at_dest) = key;
            serialize_row(leaf_node_value(dest_node, cur_at_dest), value);
        } else if (current > target_idx) {
            memcpy(destination, leaf_node_cell(old_node, cur - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, cur), LEAF_NODE_CELL_SIZE);
        }
    }

    *leaf_node_num_cells(old_node) = LEAF_NODE_SPLIT_LEFT_CELLS;
    *leaf_node_num_cells(new_node) = LEAF_NODE_SPLIT_RIGHT_CELLS;

    if (node_is_root(old_node)) {
        return creat_new_root(cursor->table, new_page_num);
    } else {
        printf("waiting for spliting internal node implementation\n");
        exit(EXIT_FAILURE);
    }
}

void create_new_root(Table *table, uint32_t right_page_num) {
    assert(table);

    Pager *pager = table->pager;
    void *root = pager_get_page(pager, table->root_page_num);
    void *right_node = pager_get_page(pager, right_page_num);

    uint32_t new_page_num = pager_get_unused_page(pager);
    void *left_node = pager_get_page(pager, new_page_num);

    memcpy(left_node, root, PAGE_SIZE);
    node_set_is_root(left_node, false);

    node_set_is_root(root, true);

}
