#include <stdlib.h>

/* test-1-uaf-read.c — definite use-after-free (heap read) */

int main() {
  int* p = (int*)malloc(sizeof(int));
  if (p == NULL) return 1;
  *p = 42;
  free(p);
  return *p;  /* line 10: use after free (read) */
}
