#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

// typedef telling compiler that, InputBuffer is an a name of a struct
// and don't ever let me say it again. (so better than struct InputBuffer imo)
typedef struct {
    char* buffer;
    size_t buffer_length; // length of mallocated bufffer
    ssize_t input_length; // length of input string
} InputBuffer;

typedef enum {
    META_COMMAND_OK,
    META_COMMAND_UNRECOGNIZED,
} MetaCommandResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT,
} StatementType;

typedef struct {
    StatementType type;
} Statement;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_FAILURE,
} PrepareStatementResult;

InputBuffer* new_input_buffer() {
    InputBuffer* inputBuffer = malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->buffer_length = 0;
    inputBuffer->input_length = 0;

    return inputBuffer;
}

void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

void print_prompt() {
    printf("db >");
}

// don't know what to do here
MetaCommandResult execute_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer); // do we still have to free if we exit?

        printf("exiting! Bye bye\n");
        // note: EXIT_SUCCESS and EXIT_FAILURE is preprocessor macro
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED;
    }
}


PrepareStatementResult prepare_statement(
    InputBuffer *input_buffer,
    Statement *statement
) {
    // strncmp compares just first n characters
    if (strncmp(input_buffer->buffer, "select", 6) == 0) {
        statement->type = STATEMENT_SELECT;
    } else if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;
    } else {
        return PREPARE_FAILURE;
    }

    return PREPARE_SUCCESS;
}

void execute_statement(Statement *statement) {
    switch (statement->type) {
        case STATEMENT_SELECT:
            printf("this is where we do select\n");
            break;
        case STATEMENT_INSERT:
            printf("this is where we do select\n");
            break;
    }
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

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    while (true) {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (execute_meta_command(input_buffer)) {
                case META_COMMAND_OK:
                    continue; // next command
                case META_COMMAND_UNRECOGNIZED:
                    printf("invalid meta command '%s'\n", input_buffer->buffer);
                    continue; // next command
            }
        }

        Statement statement; // statement not persist because who care
        switch (prepare_statement(input_buffer, &statement)) {
            case PREPARE_SUCCESS:
                break;
            case PREPARE_FAILURE:
                printf("invalid statement '%s'.\n", input_buffer->buffer);
                continue;
        }

        execute_statement(&statement);
        printf("execute.\n");
    }

}
