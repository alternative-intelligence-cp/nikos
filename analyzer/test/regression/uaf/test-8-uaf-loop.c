#include <stdlib.h>

/* test-8-uaf-loop.c — free inside a loop, access after the loop.
   The freed iteration's memory is accessed after the loop exits.
   Expected: at minimum unsafe (warning or error depending on loop analysis). */

int main() {
  int* p = NULL;
  for (int i = 0; i < 3; i++) {
    p = (int*)malloc(sizeof(int));
    if (p == NULL) return 1;
    *p = i;
    if (i == 1) {
      free(p);  /* freed on iteration i==1 */
    }
  }
  /* After the loop, p points to the last malloc (i==2, not freed).
     So this is actually safe for the last iteration. Expected: safe. */
  return *p;
}
