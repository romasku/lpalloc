#include <stdlib.h>
#include <stdio.h>

int main() {
	char *arr = malloc(100);
	for (int i = 0; i < 100; i++) {
		arr[i] = i;
	}
	for (int i = 0; i < 100; i++) {
		if (arr[i] != i) {
			printf("Data wasn't same on check:\n on position %d is %d, expected %d", i, arr[i], i);
			return 1;
		}
	}
	free(arr);
	return 0;
}
