#pragma once

#include <stdint.h>

// a trick to cast liternal number to a pointer, then get size without access address
#define size_of_attr(Struct, Attr) sizeof(((Struct*)0)->Attr)

#define COLUMN_NAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

#define PAGE_SIZE 4096
#define TABLE_MAX_PAGES 100

#define INVALID_PAGE_NUM UINT32_MAX
