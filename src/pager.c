#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pager.h"

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

// get page pointer in cache
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
