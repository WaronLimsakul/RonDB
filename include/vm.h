#pragma once

#include "statement.h"
#include "table.h"

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED,
} MetaCommandResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY,
    EXECUTE_FAILURE,
} ExecuteResult;

// don't know what to do here
MetaCommandResult execute_meta_command(InputBuffer* input_buffer, Table *table);

ExecuteResult execute_select(Table *table);

ExecuteResult execute_insert(Table *table, Row *row);

ExecuteResult execute_statement(Statement *statement, Table *table);

void serialize_row(void* destination, Row* source);

void deserialize_row(Row* destination, void *source);
