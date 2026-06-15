#include <stdlib.h>

extern int __ikos_nondet_int(void);

/* test-5-uaf-array.c — use-after-free through array indexing */

int main() {
  int* arr = (int*)calloc(5, sizeof(int));
  if (arr == NULL) return 1;

  for (int i = 0; i < 5; i++) {
    arr[i] = i * 2;
  }

  free(arr);

  /* Line 17: use after free (array indexing through freed pointer) */
  return arr[2];
}
