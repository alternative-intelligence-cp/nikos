#include <stdlib.h>

/* test-4-uaf-safe.c — allocation, use, free in correct order. Must be safe. */

int main() {
  int* p = (int*)malloc(sizeof(int));
  if (p == NULL) return 1;

  *p = 100;         /* line 9: safe write */
  int val = *p;     /* line 10: safe read */
  free(p);

  return val;
}
