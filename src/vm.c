#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "vm.h"
#include "cursor.h"
#include "b-tree.h"

static void print_constants() {
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

static void indent(uint32_t level) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
}

static void print_tree(Pager *pager, uint32_t page_num, uint32_t level) {
    void *node = pager_get_page(pager, page_num);
    indent(level);
    switch (node_type(node)) {
        case INTERNAL_NODE:
            uint32_t num_keys = *internal_node_num_keys(node);
            printf("- internal (size %u)\n", num_keys);
            for (uint32_t i = 0; i < num_keys; i++) {
                print_tree(pager, *internal_node_child(node, i), level + 1);
                indent(level + 1);
                printf("- key %d\n", *internal_node_key(node, i));
            }
            print_tree(pager, *internal_node_right_child(node), level + 1);
            break;
        case LEAF_NODE:
            uint32_t num_cells = *leaf_node_num_cells(node);
            printf("- leaf (size %d)\n", num_cells);
            for (uint32_t i = 0; i < num_cells; i ++) {
                indent(level + 1);
                printf("- %d\n", *leaf_node_key(node, i));
            }
            break;
    }
}

// don't know what to do here
MetaCommandResult execute_meta_command(InputBuffer* input_buffer, Table *table) {
    assert(input_buffer);
    assert(table);

    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer); // do we still have to free if we exit?
        // free_table(table);
        db_close(table);

        printf("exiting! Bye bye\n");
        // note: EXIT_SUCCESS and EXIT_FAILURE is preprocessor macro
        exit(EXIT_SUCCESS);
    } else if (strcmp(input_buffer->buffer, ".constants") == 0) {
        printf("Constants:\n");
        print_constants();
    } else if (strcmp(input_buffer->buffer, ".btree") == 0) {
        printf("Tree:\n");
        print_tree(table->pager, table->root_page_num, 0);
        // print_leaf_node(pager_get_page(table->pager, 0));
    } else {
        return META_COMMAND_UNRECOGNIZED;
    }

    return META_COMMAND_SUCCESS;
}

ExecuteResult execute_select(Table *table) {
    assert(table);

    Row temp_row;
    Cursor *cursor = table_start(table);
    int rows_count = 0;
    while (!cursor->end_of_table) {
        deserialize_row(&temp_row, cursor_value(cursor));
        printf(
            "id: %d | name: %s | email: %s\n",
            temp_row.id,
            temp_row.name,
            temp_row.email
        );
        cursor_advance(cursor);
        rows_count++;
    }

    // want to at least print \n if there is no row
    if (rows_count == 0) {
        printf("\n");
    }

    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Table *table, Row *row) {
    assert(table);
    assert(row);

    void* node = pager_get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    uint32_t key_to_insert = row->id;
    Cursor *cursor = table_find(table, key_to_insert);

    // catch duplicate key
    if (cursor->cell_num < num_cells) {
        uint32_t key_at_node = *leaf_node_key(node, cursor->cell_num);
        if (key_at_node == key_to_insert) {
            return EXECUTE_DUPLICATE_KEY;
        }
    }

    leaf_node_insert(cursor, key_to_insert, row);

    free(cursor);

    printf("insert 1\n");
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table) {
    assert(statement);
    assert(table);
    switch (statement->type) {
        case STATEMENT_SELECT:
            return execute_select(table);
        case STATEMENT_INSERT:
            return execute_insert(table, &statement->row_to_insert);
    }
    return EXECUTE_SUCCESS;
}

void serialize_row(void* destination, Row* source) {
    assert(source);
    assert(destination);

    memcpy(ID_OFFSET + destination, &(source->id), ID_SIZE);
    memcpy(NAME_OFFSET + destination, &(source->name), NAME_SIZE);
    memcpy(EMAIL_OFFSET + destination, &(source->email), EMAIL_SIZE);
}

void deserialize_row(Row* destination, void *source) {
    assert(source);
    assert(destination);

    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->name), source + NAME_OFFSET, NAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}
