#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define COLUMN_NAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

// a trick to cast liternal number to a pointer, then get size without access address
#define size_of_attr(Struct, Attr) sizeof(((Struct*)0)->Attr)

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
    uint32_t id; // type from stdint
    char name[COLUMN_NAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_FAILURE,
} PrepareStatementResult;

const uint32_t ID_SIZE = size_of_attr(Row, id);
const uint32_t NAME_SIZE = size_of_attr(Row, name);
const uint32_t EMAIL_SIZE = size_of_attr(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t NAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = NAME_OFFSET + NAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + NAME_SIZE + EMAIL_SIZE;

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = TABLE_MAX_PAGES * ROWS_PER_PAGE;

typedef struct {
    uint32_t num_rows; // not sure type
    Row* pages[TABLE_MAX_PAGES];
} Table;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_FAILURE,
} ExecuteResult;


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


Table *new_table(void) {
    Table *table = malloc(sizeof(Table));
    table->num_rows = 0;
    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table *table) {
    assert(table);
    // trick for round up
    int num_pages = (table->num_rows + (ROWS_PER_PAGE - 1)) / ROWS_PER_PAGE;
    for (int i = 0; i < num_pages; i++) {
        free(table->pages[i]);
    }
    free(table);
}

void print_prompt() {
    printf("db >");
}

// don't know what to do here
MetaCommandResult execute_meta_command(InputBuffer* input_buffer, Table *table) {
    assert(input_buffer);
    assert(table);

    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_input_buffer(input_buffer); // do we still have to free if we exit?
        free_table(table);

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
    assert(input_buffer);
    assert(statement);
    // strncmp compares just first n characters
    if (strcmp(input_buffer->buffer, "select") == 0) {
        statement->type = STATEMENT_SELECT;
    } else if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        statement->type = STATEMENT_INSERT;

        // start after "insert"
        int words_scanned = sscanf(
            &(input_buffer->buffer[6]),
            "%d %s %s",
            &(statement->row_to_insert.id),
            statement->row_to_insert.name,
            statement->row_to_insert.email
        );
        if (words_scanned != 3) {
            return PREPARE_SYNTAX_ERROR;
        }
    } else {
        return PREPARE_FAILURE;
    }

    return PREPARE_SUCCESS;
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

// get row address from table and row number
// malloc a new page if required
void *get_row_addr(Table *table, uint32_t row_nums) {
    assert(table);
    assert(row_nums < TABLE_MAX_ROWS);

    int page_num = row_nums / ROWS_PER_PAGE;
    void *page_addr = table->pages[page_num];
    if (page_addr == NULL) {
        page_addr = malloc(PAGE_SIZE);
        table->pages[page_num] = page_addr;
    }

    // so costly
    int row_num_in_page = row_nums % ROWS_PER_PAGE;
    int bytes_offset = row_num_in_page * ROW_SIZE;
    return page_addr + bytes_offset;
}

ExecuteResult execute_select(Table *table) {
    assert(table);

    Row temp_row;
    for (int i = 0; i < table->num_rows; i++) {
        deserialize_row(&temp_row, get_row_addr(table, i));
        printf(
            "id: %d | name: %s | email: %s \n",
            temp_row.id,
            temp_row.name,
            temp_row.email
        );
    }

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Table *table, Row *row) {
    assert(table);
    assert(row);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row *new_row = get_row_addr(table, table->num_rows);
    serialize_row(new_row, row);

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
    Table* table = new_table();

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
        }

        switch (execute_statement(&statement, table)) {
            case EXECUTE_SUCCESS:
                break;
            case EXECUTE_TABLE_FULL:
                printf("table is full");
                break;
            case EXECUTE_FAILURE:
                printf("execute command '%s' failed\n", input_buffer->buffer);
                break;
        }
    }

}
