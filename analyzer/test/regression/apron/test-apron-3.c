/*******************************************************************************
 * test-apron-3.c
 *
 * APRON smoke test: null dereference detection with apron-octagon domain.
 *
 * Verifies that the apron-octagon domain correctly identifies
 * an unconditional null dereference as an error.
 *
 * Expected: error on line 14
 ******************************************************************************/
int main() {
    int *p = 0;
    return *p; /* line 13: null dereference */
}
