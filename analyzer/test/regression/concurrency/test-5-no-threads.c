/* test-5-no-threads.c
 * Single-threaded program with no locks and no shared globals.
 * The concurrency checker should produce no warnings.
 * Expected: safe.
 */
int compute(int x) {
  return x * x + 1;
}

int main() {
  int result = compute(6);
  return result;
}
