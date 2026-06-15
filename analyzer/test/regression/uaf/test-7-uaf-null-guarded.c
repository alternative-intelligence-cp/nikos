#include <stdlib.h>

/* test-7-uaf-null-guarded.c — pointer is freed but guarded by NULL check.
   After free(p), p is not set to NULL, so the guard doesn't protect the access.
   Expected: error (the guard is on NULL, not on "freed") */

int main() {
  int* p = (int*)malloc(sizeof(int));
  if (p == NULL) return 1;
  *p = 7;
  free(p);

  /* p is not NULL (still holds old address), so guard does NOT help */
  if (p != NULL) {
    return *p;  /* line 15: use after free — the NULL check is insufficient */
  }

  return 0;
}
