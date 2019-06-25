#include <stdlib.h>
#include <stdio.h>
#include "../allocator.h"

#define maxn 1000000

int z = 0;
int *arrs[maxn];
size_t sizes[maxn];

int main() {
    srand(0);
	for (int q = 0; q < 100000; q++) {
	    printf("Entries cnt %d\n", z);
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
    }
	return 0;
}
