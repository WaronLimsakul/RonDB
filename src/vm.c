#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "vm.h"
#include "cursor.h"

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
    } else {
        return META_COMMAND_UNRECOGNIZED;
    }
}

ExecuteResult execute_select(Table *table) {
    assert(table);

    Row temp_row;
    Cursor *cursor = table_start(table);
    while (!cursor->end_of_table) {
        deserialize_row(&temp_row, cursor_value(cursor));
        printf(
            "id: %d | name: %s | email: %s\n",
            temp_row.id,
            temp_row.name,
            temp_row.email
        );
        cursor_advance(cursor);
    }

    // want to at least print \n if there is no row
    if (table->num_rows == 0) {
        printf("\n");
    }

    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Table *table, Row *row) {
    assert(table);
    assert(row);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Cursor *end_cursor = table_end(table);
    Row *new_row_addr = cursor_value(end_cursor);
    serialize_row(new_row_addr, row);

    free(end_cursor); // ?

    table->num_rows ++;
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
