#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "../include/statement.h"

InputBuffer* new_input_buffer() {
    InputBuffer* inputBuffer = malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->buffer_length = 0;
    inputBuffer->input_length = 0;

    return inputBuffer;
}

void close_input_buffer(InputBuffer* input_buffer) {
    assert(input_buffer);
    free(input_buffer->buffer);
    free(input_buffer);
}

PrepareStatementResult prepare_statement(
    InputBuffer *input_buffer,
    Statement *statement
) {
    assert(input_buffer);
    assert(statement);
    if (strcmp(input_buffer->buffer, "select") == 0) {
        statement->type = STATEMENT_SELECT;

    // strncmp compares just first n characters
    } else if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;

        // this function trim the first param until the second param
        // then return the trimmed part.
        strtok(input_buffer->buffer, " ");

        // you can call it again with NULL and it will use the same
        // string (but trimmed) as first param again. cool
        char *id_str = strtok(NULL, " ");
        char *name = strtok(NULL, " ");
        char *email = strtok(NULL, " ");

        if (!(id_str && name && email)) {
            return PREPARE_SYNTAX_ERROR;
        }

        if (strlen(name) > COLUMN_NAME_SIZE || strlen(email) > COLUMN_EMAIL_SIZE) {
            return PREPARE_INPUT_TOO_LONG;
        }

        int id = atoi(id_str);
        if (id < 0) {
            return PREPARE_NEGATIVE_ID;
        }

        // but id should be uint32 ?
        statement->row_to_insert.id = id;
        strcpy(statement->row_to_insert.name, name);
        strcpy(statement->row_to_insert.email, email);
    } else {
        return PREPARE_FAILURE;
    }

    return PREPARE_SUCCESS;
}

void read_input(InputBuffer* input_buffer) {
    assert(input_buffer);
    // getline will get entire line from stream you put (in this case stdin)
    // also, if the string is not malloced, it will do so for you.
    ssize_t bytes_read =
        getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("error reading command \n");
        exit(EXIT_FAILURE);
    }

    // getline reads until \n. so we gotta trim that out
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = '\0'; // from \n to \0
}
