/*******************************************************************************
 * test-apron-2.c
 *
 * APRON smoke test: safe loop with var-pack-apron-octagon domain.
 *
 * Verifies that the var-pack-apron-octagon domain correctly identifies
 * a safe loop. The var-pack domains partition variables into independent
 * packs for scalability, exercising that decomposition path.
 *
 * Expected: safe with var-pack-apron-octagon
 ******************************************************************************/
void fill(int *buf, int n) {
    for (int i = 0; i < n; i++) {
        buf[i] = i * 2;
    }
}

int main() {
    int data[8];
    fill(data, 8);
    return data[7];
}
