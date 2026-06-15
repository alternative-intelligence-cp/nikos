/* test-8-heap-race.c
 * Heap-allocated mutex exists but is NOT acquired before accessing
 * the global — unprotected access.
 * Expected: unsafe (data race warning).
 */
#include <pthread.h>
#include <stdlib.h>

int g_data = 0;

int main() {
  pthread_mutex_t* lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (!lock) return 1;
  pthread_mutex_init(lock, NULL);

  /* Intentionally forget to lock */
  g_data = 99; /* line 17: unprotected write — data race warning expected */

  pthread_mutex_destroy(lock);
  free(lock);
  return g_data;
}
