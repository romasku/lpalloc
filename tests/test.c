#include <stdlib.h>
#include <stdio.h>
#include "../allocator.h"

#define MAXN 1000000
#define CHECKS 1000
#define ALLOC_MAX_SIZE 500000
#define EACH_OPERATION_MAX_CNT 20
#define ARRAY_FILL_VALUE 0xffffffff

int allocted_cnt = 0;
int *arrs[MAXN];
size_t sizes[MAXN];

int min(int a, int b) {
    return a < b ? a : b;
}

int main() {
    srand(0);
	for (int checks = 0; checks < CHECKS; checks++) {
	    printf("Step %d\n", checks);
	    int allocs_cnt = rand() % EACH_OPERATION_MAX_CNT;
        for (int i = 0; i < allocs_cnt; i++) {
            int alloc_size = rand() % ALLOC_MAX_SIZE;
            arrs[allocted_cnt] = malloc(alloc_size * sizeof(int));
            sizes[allocted_cnt] = alloc_size;
            for (int j = 0; j < alloc_size; j++) {
                arrs[allocted_cnt][j] = ARRAY_FILL_VALUE;
            }
            allocted_cnt++;
        }

        int free_cnt = rand() % EACH_OPERATION_MAX_CNT;
        if (allocted_cnt < free_cnt) {
            free_cnt = allocted_cnt;
        }
        for (int i = 0; i < free_cnt; i++) {
            int del_index = rand() % allocted_cnt;
            allocted_cnt--;
            for (int j = 0; j < sizes[del_index]; j++) {
                if (arrs[del_index][j] != ARRAY_FILL_VALUE) {
                    printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[del_index][j],
                           ARRAY_FILL_VALUE);
                    return 1;
                }
            }
            free(arrs[del_index]);
            for (int j = del_index; j < allocted_cnt; j++) {
                arrs[j] = arrs[j + 1];
                sizes[j] = sizes[j + 1];
            }
        }

        int realloc_cnt = rand() % EACH_OPERATION_MAX_CNT;
        if (allocted_cnt < realloc_cnt) {
            realloc_cnt = allocted_cnt;
        }
        for (int i = 0; i < realloc_cnt; i++) {
            int realloc_index = rand() % allocted_cnt;
            int realloc_size = rand() % ALLOC_MAX_SIZE;
            arrs[realloc_index] = realloc(arrs[realloc_index], realloc_size * sizeof(int));
            int min_size = min(realloc_size, sizes[realloc_index]);
            sizes[realloc_index] = realloc_size;
            for (int j = 0; j < min_size; j++) {
                if (arrs[realloc_index][j] != ARRAY_FILL_VALUE) {
                    printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[realloc_index][j],
                           ARRAY_FILL_VALUE);
                    return 1;
                }
            }
            for (int j = 0; j < realloc_size; j++) {
                arrs[realloc_index][j] = ARRAY_FILL_VALUE;
            }
        }
    }
	return 0;
}
