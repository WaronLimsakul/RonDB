#pragma once

#include "config.h"
#include "pager.h"

typedef struct {
    uint32_t id; // type from stdint
    char name[COLUMN_NAME_SIZE + 1]; // the last one for \0
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

extern const uint32_t ID_SIZE;
extern const uint32_t NAME_SIZE;
extern const uint32_t EMAIL_SIZE;
extern const uint32_t ID_OFFSET;
extern const uint32_t NAME_OFFSET;
extern const uint32_t EMAIL_OFFSET;
extern const uint32_t ROW_SIZE;

extern const uint32_t ROWS_PER_PAGE;
extern const uint32_t TABLE_MAX_ROWS;

typedef struct {
    uint32_t num_rows; // not sure type
    Pager *pager;
} Table;

Table *db_open(char *file_name);

void db_close(Table *table);
