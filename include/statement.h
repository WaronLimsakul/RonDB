#pragma once

#include <unistd.h>

#include "../include/table.h"

// typedef telling compiler that, InputBuffer is an a name of a struct
// and don't ever let me say it again. (so better than struct InputBuffer imo)
typedef struct {
    char* buffer;
    size_t buffer_length; // length of mallocated bufffer
    ssize_t input_length; // length of input string
} InputBuffer;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_INPUT_TOO_LONG,
    PREPARE_NEGATIVE_ID,
    PREPARE_FAILURE,
} PrepareStatementResult;

InputBuffer* new_input_buffer(void);

void close_input_buffer(InputBuffer* input_buffer);

PrepareStatementResult prepare_statement(
    InputBuffer *input_buffer,
    Statement *statement
);

void read_input(InputBuffer* input_buffer);
