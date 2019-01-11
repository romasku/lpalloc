#include <stdlib.h>
#include <stdio.h>

int main() {
	char *arrs[100];
	for (int i = 0; i < 100; i++) {
		arrs[i] = malloc(100);
		for (int j = 0; j < 100; j++) {
			arrs[i][j] = j;
		}
	}
	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			if (arrs[i][j] != j) {
				printf("Data wasn't same on check:\n on position %d is %d, expected %d", j, arrs[i], j);
				return 1;
			}
		}
		free(arrs[i]);
	}
	return 0;
}
