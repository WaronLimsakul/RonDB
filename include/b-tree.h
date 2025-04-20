#pragma once

#include <stdint.h>
#include "table.h"
#include "cursor.h"

// common node header consts
extern const uint32_t NODE_TYPE_SIZE;  // dedicate 1 byte for that. easy but wasty
extern const uint32_t NODE_TYPE_OFFSET;
extern const uint32_t IS_ROOT_SIZE; // again wasty but easy
extern const uint32_t IS_ROOT_OFFSET;
extern const uint32_t PARENT_POINTER_SIZE;
extern const uint32_t PARENT_POINTER_OFFSET;
extern const uint32_t COMMON_NODE_HEADER_SIZE;
// additional leaf node header
extern const uint32_t LEAF_NODE_NUM_CELLS_SIZE;
extern const uint32_t LEAF_NODE_NUM_CELLS_OFFSET;
extern const uint32_t LEAF_NODE_NEXT_LEAF_SIZE;
extern const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET;
extern const uint32_t LEAF_NODE_HEADER_SIZE;
// leaf node body const
extern const uint32_t LEAF_NODE_KEY_SIZE;
extern const uint32_t LEAF_NODE_KEY_OFFSET;
extern const uint32_t ROW_SIZE;
extern const uint32_t LEAF_NODE_VALUE_SIZE;
extern const uint32_t LEAF_NODE_VALUE_OFFSET;

extern const uint32_t LEAF_NODE_CELL_SIZE;
extern const uint32_t LEAF_NODE_SPACE_FOR_CELLS;
extern const uint32_t LEAF_NODE_MAX_CELLS;

extern const uint32_t LEAF_NODE_SPLIT_LEFT_CELLS;
extern const uint32_t LEAF_NODE_SPLIT_RIGHT_CELLS;

typedef enum {
    INTERNAL_NODE,
    LEAF_NODE,
} NodeType;

int32_t* leaf_node_num_cells(void *node);
int32_t* leaf_node_next_leaf(void *node);
void *leaf_node_cell(void *node, int32_t cell_num);
uint32_t *leaf_node_key(void *node, int32_t cell_num);
void *leaf_node_value(void *node, int32_t cell_num);
void init_leaf_node(void *node);
void leaf_node_insert(Cursor *cursor, uint32_t key, Row *value);
Cursor *leaf_node_find(Table *table, uint32_t page_num, uint32_t key);
NodeType node_type(void *node);
void set_node_type(void *node, NodeType type);
void node_set_is_root(void *node, bool is_root);
void leaf_node_split_and_insert(Cursor *cursor, uint32_t key, Row *value);
void create_new_root(Table *table, uint32_t right_page_num);

uint32_t *internal_node_num_keys(void *node);
uint32_t *internal_node_right_child(void *node);
void *internal_node_cell(void *node, uint32_t cell_num);
// child is at the front of the cell;
uint32_t *internal_node_child(void *node, uint32_t child_num);
uint32_t *internal_node_key(void *node, uint32_t cell_num);
void init_internal_node(void *node);
uint32_t get_node_max_key(void *node);

Cursor *internal_node_find(Table *table, uint32_t page_num, uint32_t key);
uint32_t internal_node_find_child(void *node, uint32_t key);
