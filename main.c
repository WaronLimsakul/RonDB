#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

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
    char name[COLUMN_NAME_SIZE + 1]; // the last one for \0
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

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
    int file_descriptor;
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t num_rows; // not sure type
    Pager *pager;
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


Table *new_table(Pager *pager) {
    Table *table = malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = 0;
    // for (int i = 0; i < TABLE_MAX_PAGES; i++) {
    //     table->pages[i] = NULL;
    // }
    return table;
}

Pager *open_pager(char *file_name) {
    // the program can
    // 1. read and write the file
    // 2. create if the file doesn't exist
    // 3. let user read the file
    // 4. let user write the file
    int fd = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        printf("cannot open file '%s'\n", file_name);
        exit(EXIT_FAILURE);
    }

    Pager *new_pager = malloc(sizeof(Pager));
    new_pager->file_descriptor = fd;

    off_t file_length = lseek(fd, 0, SEEK_END);
    new_pager->file_length = file_length;

    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        new_pager->pages[i] = NULL;
    }
    return new_pager;
}

Table *db_open(char *file_name) {

    Pager *pager = open_pager(file_name);
    Table *table = malloc(sizeof(Table));

    table->num_rows = pager->file_length / ROW_SIZE;
    table->pager = pager;
    return table;
}

void pager_flush(Pager* pager, int page_num, size_t size) {
    assert(pager);
    assert(page_num < TABLE_MAX_PAGES);

    int fd = pager->file_descriptor;

    off_t seek_result = lseek(fd, PAGE_SIZE * page_num, SEEK_SET);
    if (seek_result == -1) {
        printf("error flushing page number: %d (seeking)\n", page_num);
        exit(EXIT_FAILURE);
    }

    ssize_t write_result = write(fd, pager->pages[page_num], size);
    if (write_result == -1) {
        printf("error flushing page number: %d (writing)\n", page_num);
        exit(EXIT_FAILURE);
    }
}

void db_close(Table *table) {
    assert(table);

    Pager *pager = table->pager;
    // deal with full pages first
    int num_full_pages = table->num_rows / ROWS_PER_PAGE;

    for (int i = 0; i < num_full_pages; i++) {
        pager_flush(pager, i, PAGE_SIZE);
    }
    // deal with partial page
    int partial_page_rows = table->num_rows % ROWS_PER_PAGE;
    if (partial_page_rows > 0) {
        pager_flush(pager, num_full_pages, ROW_SIZE * partial_page_rows);
    }

    int close_status = close(pager->file_descriptor);
    if (close_status == -1) {
        printf("error closing db connection\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < TABLE_MAX_PAGES; i++) {
        if (pager->pages[i] != NULL) {
            free(pager->pages[i]);
        }
    }
    free(pager);
    free(table);
}

void print_prompt() {
    printf("RonDB >");
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

void *pager_get_page(Pager *pager, int page_num) {
    assert(pager);
    assert(page_num < TABLE_MAX_PAGES);

    // cache miss: allocate a page cache + load from file
    if (pager->pages[page_num] == NULL) {
        void *new_page = malloc(PAGE_SIZE);

        // how many pages (round up). might get int overflow.
        int num_pages = (pager->file_length + (PAGE_SIZE - 1)) / PAGE_SIZE;

        if (page_num <= num_pages) {
            // go the start of the page
            lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
            // read that page to the allocated memory
            ssize_t bytes_read = read(pager->file_descriptor, new_page, PAGE_SIZE);
            if (bytes_read == -1) {
                printf("error reading the file\n");
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = new_page;
    }

    return pager->pages[page_num];
}

// get row address from table and row number
// malloc a new page if required
void *table_get_row(Table *table, uint32_t row_nums) {
    assert(table);
    assert(row_nums < TABLE_MAX_ROWS);

    int page_num = row_nums / ROWS_PER_PAGE;
    void *page_addr = pager_get_page(table->pager, page_num);

    int row_num_in_page = row_nums % ROWS_PER_PAGE;
    int bytes_offset = row_num_in_page * ROW_SIZE;
    return page_addr + bytes_offset;
}

ExecuteResult execute_select(Table *table) {
    assert(table);

    Row temp_row;
    int i = 0;
    for (; i < table->num_rows; i++) {
        deserialize_row(&temp_row, table_get_row(table, i));
        printf(
            "id: %d | name: %s | email: %s\n",
            temp_row.id,
            temp_row.name,
            temp_row.email
        );
    }

    // want to at least print \n if there is no row
    if (i == 0) {
        printf("\n");
    }

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Table *table, Row *row) {
    assert(table);
    assert(row);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row *new_row = table_get_row(table, table->num_rows);
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
