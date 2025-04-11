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

InputBuffer* new_input_buffer() {
    InputBuffer* inputBuffer = malloc(sizeof(InputBuffer));
    inputBuffer->buffer = NULL;
    inputBuffer->buffer_length = 0;
    inputBuffer->input_length = 0;

    return inputBuffer;
}

void print_prompt() {
    printf("db >");
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

void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();
    while (true) {
        print_prompt();
        read_input(input_buffer);

        // check if user want to exit
        if (strcmp(input_buffer->buffer, ".exit") == 0) {
            close_input_buffer(input_buffer); // do we still have to free if we exit?

            // note: EXIT_SUCCESS and EXIT_FAILURE is preprocessor macro
            exit(EXIT_SUCCESS);
        } else {
            printf("Unrecognized command '%s' .\n", input_buffer->buffer);
        }
    }

}


// step 1
// should be able to
// 1. input somethign and got unrecognized "error"
// 2. or if .exit -> exit
