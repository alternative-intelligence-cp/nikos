/*******************************************************************************
 * test-apron-1.c
 *
 * APRON smoke test: safe array access with apron-octagon domain.
 *
 * Verifies that the apron-octagon domain correctly identifies
 * a trivially safe loop as safe (no buffer overflow).
 *
 * Expected: safe with apron-octagon
 ******************************************************************************/
int main() {
    int buf[10];
    for (int i = 0; i < 10; i++) {
        buf[i] = i;
    }
    return buf[5];
}
