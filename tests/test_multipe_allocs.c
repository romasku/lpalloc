#include <stdlib.h>
#include <stdio.h>
#include "../allocator.h"

int main() {
	char *arrs[100];
	for (int q = 0; q < 1000; q++) {
        for (int i = 0; i < 100; i++) {
            arrs[i] = malloc(100);
            //printf("Malloc num %d\n", i);
            for (int j = 0; j < 100; j++) {
                arrs[i][j] = j;
            }
        }
        for (int i = 10; i < 90; i++) {
            for (int j = 0; j < 100; j++) {
                if (arrs[i][j] != j) {
                    printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[i][j], j);
                    return 1;
                }
            }
            free(arrs[i]);
            //printf("Free num %d\n", i);
        }
    }
	return 0;
}
