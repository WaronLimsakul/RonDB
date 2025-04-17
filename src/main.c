#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/statement.h"
#include "../include/vm.h"
#include "../include/table.h"

void print_prompt() {
    printf("RonDB >");
}


int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    // we need db file name
    if (argc != 2) {
        printf("need db file name\n");
        exit(EXIT_FAILURE);
    }

    char *file_name = argv[1];
    Table* table = db_open(file_name);

    while (true) {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (execute_meta_command(input_buffer, table)) {
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
            case PREPARE_SYNTAX_ERROR:
                printf("syntax error for statement: '%s'.\n", input_buffer->buffer);
                continue;
            case PREPARE_INPUT_TOO_LONG:
                printf("error: input is too long\n");
                continue;
            case PREPARE_NEGATIVE_ID:
                printf("error: id is negative\n");
                continue;
        }

        switch (execute_statement(&statement, table)) {
            case EXECUTE_SUCCESS:
                break;
            case EXECUTE_TABLE_FULL:
                printf("error: table is full\n");
                break;
            case EXECUTE_FAILURE:
                printf("execute command '%s' failed\n", input_buffer->buffer);
                break;
        }
    }

}
