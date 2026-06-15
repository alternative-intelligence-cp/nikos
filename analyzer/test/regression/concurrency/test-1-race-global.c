/* test-1-race-global.c
 * Unprotected read+write to a global variable — no mutex held.
 * Expected: data race warning (unsafe).
 */
#include <pthread.h>

int g_counter = 0; /* shared global */

void* worker(void* arg) {
  g_counter++; /* line 10: unprotected write */
  return NULL;
}

int main() {
  pthread_t t;
  pthread_create(&t, NULL, worker, NULL);
  g_counter++; /* line 17: unprotected write */
  pthread_join(t, NULL);
  return 0;
}
