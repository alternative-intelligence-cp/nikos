/*******************************************************************************
 * test-apron-1-unsafe.c
 *
 * APRON smoke test: detected buffer overflow with apron-octagon domain.
 *
 * Verifies that the apron-octagon domain correctly identifies
 * an obvious out-of-bounds access as an error.
 *
 * Expected: error on line 15
 ******************************************************************************/
int main() {
    int buf[10];
    for (int i = 0; i < 10; i++) {
        buf[i] = i;
    }
    buf[10] = 99; /* line 16: out-of-bounds — index 10, size 10 */
    return 0;
}
