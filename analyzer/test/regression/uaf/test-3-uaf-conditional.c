#include <stdlib.h>

extern int __ikos_nondet_int(void);

/* test-3-uaf-conditional.c — conditional free, then access (possible UAF) */

int main() {
  int* p = (int*)malloc(sizeof(int));
  if (p == NULL) return 1;
  *p = 10;

  if (__ikos_nondet_int()) {
    free(p);
  }

  return *p;  /* line 16: possible use after free (warning or error) */
}
