#include "b-tree.h"
#include "cursor.h"
#include "vm.h"
#include "config.h"

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
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET =
    LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NUM_CELLS_OFFSET;
const uint32_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE;

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

// easy to test
const uint32_t INTERNAL_NODE_MAX_CELLS = 3;

// only check the first 8 bits
NodeType node_type(void *node) {
    assert(node);
    uint8_t value = *(uint8_t*)(node + NODE_TYPE_OFFSET);
    return (NodeType)value;
}

// only change the first 8 bits
void node_set_type(void *node, NodeType type) {
    assert(node);
    *(uint8_t*)(node + NODE_TYPE_OFFSET) = (uint8_t)type;
}

bool node_is_root(void *node) {
    assert(node);
    // remember to cast to 8 bits first, so we can pick that 8 bits
    uint8_t val = *(uint8_t *)(node + IS_ROOT_OFFSET);
    return (bool)val;
}

void node_set_is_root(void *node, bool is_root) {
    assert(node);
    *(uint8_t *)(node + IS_ROOT_OFFSET) = (uint8_t)is_root;
}

uint32_t *node_parent(void *node) {
    assert(node);
    return node + PARENT_POINTER_OFFSET;
}

int32_t* leaf_node_num_cells(void *node) {
    assert(node);
    return node + LEAF_NODE_NUM_CELLS_OFFSET;
}

int32_t *leaf_node_next_leaf(void *node) {
    assert(node);
    return node + LEAF_NODE_NEXT_LEAF_OFFSET;
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
    node_set_type(node, LEAF_NODE);
    *leaf_node_num_cells(node) = 0;
    *leaf_node_next_leaf(node) = 0; // default to 0 (which should be root)
    node_set_is_root(node, false);
}

uint32_t *internal_node_num_keys(void *node) {
    assert(node);
    return node + INTERNAL_NODE_NUM_KEYS_OFFSET;
}

uint32_t *internal_node_right_child(void *node) {
    assert(node);
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

void *internal_node_cell(void *node, uint32_t cell_num) {
    assert(node);
    // assert(cell_num < *internal_node_num_keys(node)); I want to access the back sometimes

    return node + INTERNAL_NODE_HEADER_SIZE + (INTERNAL_NODE_CELL_SIZE * cell_num);
}

// child is at the front of the cell;
uint32_t *internal_node_child(void *node, uint32_t child_num) {
    uint32_t num_keys = internal_node_num_keys(node);
    if (child_num > num_keys) {
        printf("tried to access child_num: %u > %u :num_keys\n", num_keys, child_num);
        exit(EXIT_FAILURE);
    }

    // right most child case
    if (child_num == *internal_node_num_keys(node)) {
        uint32_t *right_child = internal_node_right_child(node);
        if (*right_child == INVALID_PAGE_NUM) {
            printf("tried to access the right child but invalid page. num_keys: %u, child_num: %u\n", num_keys, child_num);
            exit(EXIT_FAILURE);
        }
        return right_child;
    }

    // inside cell case
    uint32_t *node_cell = internal_node_cell(node, child_num);
    if (*node_cell == INVALID_PAGE_NUM) {
        printf("tried to access child_num: %u but invalid page\n", child_num)
        exit(EXIT_FAILURE);
    }
    return node_cell;
}

uint32_t *internal_node_key(void *node, uint32_t cell_num) {
    return (void *)internal_node_cell(node, cell_num) + INTERNAL_NODE_CHILD_SIZE;
}

void init_internal_node(void *node) {
    assert(node);

    node_set_type(node, INTERNAL_NODE);
    *internal_node_num_keys(node) = 0;
    node_set_is_root(node, false);
    // if not do this, we might got root as a right child
    *internal_node_right_child(node) = INVALID_PAGE_NUM;
}

uint32_t get_node_max_key(Pager *pager, void *node) {
    assert(node);

    switch (node_type(node)) {
        case INTERNAL_NODE:
            // go right
            uint32_t right_page_num = internal_node_right_child(node);
            void *rigth_child = pager_get_page(pager, right_page_num);
            return get_node_max_key(pager, right_child);
        case LEAF_NODE:
            return *leaf_node_key(node, *leaf_node_num_cells(node) - 1);
        default:
            printf("error: node has no type\n");
            exit(EXIT_FAILURE);
    }
}

void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value) {
    assert(cursor);
    assert(value);

    void *node = pager_get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    if (num_cells >= LEAF_NODE_MAX_CELLS) {
        leaf_node_split_and_insert(cursor, key, value);
        return;
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

static void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key) {
    uint32_t target_cell_num = internal_node_find_child(node, old_key);
    // if the child is the right most, who care? we can just set it.
    *internal_node_key(node, target_cell_num) = new_key;
}

static void internal_node_split_and_insert(
    Table *table,
    uint32_t parent_page_num,
    uint32_t child_page_num
);

// plan:
// 1. check if the node is full
// 2. check if the node is empty
// 3. check if I have to shift and write
// or put right child at the back and replace the right child
// 4. do the write
// 5. increment num keys
static void internal_node_insert(
    Table *table,
    uint32_t parent_page_num,
    uint32_t child_page_num
) {
    assert(table);

    Pager *pager = table->pager;
    void *parent_node = pager_get_page(pager, parent_page_num);
    void *child_node = pager_get_page(pager, child_page_num);

    // 1. check if node is full
    uint32_t original_num_keys = *internal_node_num_keys(parent_node);
    if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
        internal_node_split_and_insert(pager, parent_page_num, child_page_num);
        return;
    }

    // 2. check if node is empty (right child num is INVALID_PAGE_NUM),
    // if it is, just put the right child
    uint32_t right_child_page_num = *internal_node_right_child(parent_node);
    if (right_child_page_num == INVALID_PAGE_NUM) {
        *(internal_node_right_child(parent_node)) = child_page_num;
        return;
    }

    // prepare for check step 3
    uint32_t child_max_key = get_node_max_key(child_node);

    void *right_child = pager_get_page(pager, right_child_page_num);
    uint32_t right_child_max_key = get_node_max_key(right_child);

    // !!! important !!! you need to increment first before doing inserting logic
    // so that when write at original_num_keys position, we actually write at
    // the end of body and not the right most
    *internal_node_num_keys(parent_node) = original_num_keys + 1;

    // 2. check if we shift or just add back
    if (child_max_key > right_child_max_key) {
        // move the old right child back to the end of body
        *internal_node_child(parent_node, original_num_keys) = right_child_page_num;
        *internal_node_key(parent_node, original_num_keys) = right_child_max_key;
        // move the new child to replace old right child
        *internal_node_right_child(parent_node) = child_page_num;
    } else {
        uint32_t target_idx = internal_node_find_child(parent_node, child_max_key);

        // shift to the right to give room for insert
        for (uint32_t pos = original_num_keys; pos > target_idx; pos--) {
            void *dest = internal_node_cell(parent_node, pos);
            void *src = internal_node_cell(parent_node, pos - 1);
            memcpy(dest, src, INTERNAL_NODE_CELL_SIZE);
        }

        *internal_node_child(parent_node, target_idx) = child_page_num;
        *internal_node_key(parent_node, target_idx) = child_max_key;
    }
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
    assert(cursor);
    assert(value);

    Pager *pager = cursor->table->pager;
    assert(pager);
    uint32_t old_page_num = cursor->page_num;
    void *old_node = pager_get_page(pager, old_page_num);
    assert(old_node);

    // current key of the node we will split. will use this when update the parent
    uint32_t old_key = get_node_max_key(old_node);

    // malloc new page
    int new_page_num = pager_get_unused_page(pager);
    void *new_node = pager_get_page(pager, new_page_num);
    init_leaf_node(new_node);

    // set next leaf for both nodes
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    // set parent for new node. not sure what will happen if old_node is root
    *node_parent(new_node) = *node_parent(old_node);

    // splitting data
    uint32_t target_idx = cursor->cell_num;
    for (int32_t cur = LEAF_NODE_MAX_CELLS; cur >= 0; cur--) {
        void *dest_node;
        if (cur < LEAF_NODE_SPLIT_LEFT_CELLS) {
            dest_node = old_node;
        } else {
            dest_node = new_node;
        }

        uint32_t cur_at_dest = cur % LEAF_NODE_SPLIT_LEFT_CELLS;
        void *destination = leaf_node_cell(dest_node, cur_at_dest);
        assert(destination);

        if (cur == target_idx) {
            *leaf_node_key(dest_node, cur_at_dest) = key;
            serialize_row(leaf_node_value(dest_node, cur_at_dest), value);
        } else if (cur > target_idx) {
            memcpy(destination, leaf_node_cell(old_node, cur - 1), LEAF_NODE_CELL_SIZE);
        } else {
            memcpy(destination, leaf_node_cell(old_node, cur), LEAF_NODE_CELL_SIZE);
        }
    }

    // update number of cells in each node
    *leaf_node_num_cells(old_node) = LEAF_NODE_SPLIT_LEFT_CELLS;
    *leaf_node_num_cells(new_node) = LEAF_NODE_SPLIT_RIGHT_CELLS;

    // update / create parent
    if (node_is_root(old_node)) {
        return create_new_root(cursor->table, new_page_num);
    } else {
        uint32_t parent_page_num = *node_parent(old_node);
        void *parent_node = pager_get_page(pager, parent_page_num);

        uint32_t new_key = get_node_max_key(old_node);
        update_internal_node_key(parent_node, old_key, new_key);
        internal_node_insert(cursor->table, parent_page_num, new_page_num);
    }
}

// only happen when split leaf that is originally root
void create_new_root(Table *table, uint32_t right_page_num) {
    assert(table);

    Pager *pager = table->pager;
    void *root = pager_get_page(pager, table->root_page_num);
    void *right_node = pager_get_page(pager, right_page_num);

    uint32_t new_page_num = pager_get_unused_page(pager);
    void *left_node = pager_get_page(pager, new_page_num);

    memcpy(left_node, root, PAGE_SIZE);
    node_set_is_root(left_node, false);

    *node_parent(left_node) = table->root_page_num;
    *node_parent(right_node) = table->root_page_num;

    init_internal_node(root); // root become internal node
    node_set_is_root(root, true);
    *internal_node_num_keys(root) = 1; // when create root, we split to 2 nodes
    *internal_node_right_child(root) = right_page_num; // rightmost is the right
    *internal_node_child(root, 0) = new_page_num; // first child is the left
    *internal_node_key(root, 0) = get_node_max_key(left_node); // key is the max of left
}

static bool high_key_internal(void *node, uint32_t key_num, uint32_t target) {
    return *internal_node_key(node, key_num) >= target;
}

uint32_t internal_node_find_child(void *node, uint32_t key) {
    assert(node);

    uint32_t num_keys = *internal_node_num_keys(node);

    uint32_t low = 0;
    uint32_t high = num_keys;

    while(low < high) {
        uint32_t mid = low + ((high - low) / 2);
        if (high_key_internal(node, mid, key)) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    return low;
}

// get the cursor that points to the place we can insert (eventually, leaf node)
Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key) {
    assert(table);
    void *node = pager_get_page(table->pager, page_num);

    uint32_t target_idx = internal_node_find_child(node, key);
    uint32_t child_page_num = *internal_node_child(node, target_idx);
    void *target_child = pager_get_page(table->pager, child_page_num);

    switch (node_type(target_child)) {
        case INTERNAL_NODE:
            return internal_node_find(table, child_page_num, key);
        case LEAF_NODE:
            return leaf_node_find(table, child_page_num, key);
    }
    printf("error: node has no type\n");
    exit(EXIT_FAILURE);
    return NULL;
}

static void internal_node_split_and_insert(
    Table *table,
    uint32_t parent_page_num,
    uint32_t child_page_num,
) {
    assert(table);
    uint32_t old_page_num = parent_page_num;
    uint32_t old_paeg_max =
}
