#include <stdlib.h>

/* test-6-uar.c — use-after-return: return pointer to local stack variable */

static int* get_local() {
  int x = 42;
  return &x;   /* pointer to local variable escapes */
}

int main() {
  int* p = get_local();
  return *p;   /* line 12: use after return */
}
