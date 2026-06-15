#include <stdlib.h>

/* test-2-uaf-write.c — definite use-after-free (heap write) */

int main() {
  int* p = (int*)malloc(sizeof(int));
  if (p == NULL) return 1;
  free(p);
  *p = 99;  /* line 9: use after free (write) */
  return 0;
}
