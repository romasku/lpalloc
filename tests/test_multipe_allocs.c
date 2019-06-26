#include <stdlib.h>
#include <stdio.h>
#include "../allocator.h"

#define maxn 1000000

int z = 0;
int *arrs[maxn];
size_t sizes[maxn];

int min(int a, int b) {
    return a < b ? a : b;
}

int main() {
    srand(0);
	for (int q = 0; q < 1000; q++) {
	    printf("Step %d\n", q);
	    int allocs_cnt = rand() % 20;
        for (int i = 0; i < allocs_cnt; i++) {
            int alloc_size = rand() % (500000) + 10;
            arrs[z] = malloc(alloc_size * sizeof(int));
            sizes[z] = alloc_size;
            for (int j = 0; j < alloc_size; j++) {
                arrs[z][j] = 0xffffffff;
            }
            z++;
        }

        int free_cnt = rand() % 20;
        if (z < free_cnt) {
            free_cnt = z;
        }
        for (int i = 0; i < free_cnt; i++) {
            int del_index = rand() % z;
            z--;
            for (int j = 0; j < sizes[del_index]; j++) {
                if (arrs[del_index][j] !=  0xffffffff) {
                    printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[del_index][j], 0xffffffff);
                    return 1;
                }
            }
            free(arrs[del_index]);
            for (int j = del_index; j < z; j++) {
                arrs[j] = arrs[j + 1];
                sizes[j] = sizes[j + 1];
            }
        }

        int realloc_cnt = rand() % 20;
        if (z < realloc_cnt) {
            realloc_cnt = z;
        }
        for (int i = 0; i < realloc_cnt; i++) {
            int realloc_index = rand() % z;
            int realloc_size = rand() % (500000) + 10;
            arrs[realloc_index] = realloc(arrs[realloc_index], realloc_size * sizeof(int));
            int min_size = min(realloc_size, sizes[realloc_index]);
            sizes[realloc_index] = realloc_size;
            for (int j = 0; j < min_size; j++) {
                if (arrs[realloc_index][j] !=  0xffffffff) {
                    printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[realloc_index][j], 0xffffffff);
                    return 1;
                }
            }
            for (int j = 0; j < realloc_size; j++) {
                arrs[realloc_index][j] = 0xffffffff;
            }
        }
    }
	return 0;
}
