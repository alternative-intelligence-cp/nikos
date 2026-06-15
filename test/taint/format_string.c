/*
 * Format string vulnerability test: getenv() -> printf(tainted_fmt)
 *
 * Expected: UNSAFE (FormatString at printf call)
 */
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    /* Attacker-controlled format string passed directly to printf */
    const char* fmt = getenv("LOG_FMT");
    if (!fmt) return 1;

    printf(fmt); /* UNSAFE: FormatString -- tainted format string */
    return 0;
}
