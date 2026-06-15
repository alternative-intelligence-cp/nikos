/* test-7-heap-mutex.c
 * Heap-allocated mutex protecting a global variable.
 * 12-B extends the checker to track DynAllocMemoryLocation locks.
 * Expected: safe (all accesses hold the heap mutex).
 */
#include <pthread.h>
#include <stdlib.h>

int g_data = 0;

int main() {
  pthread_mutex_t* lock = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  if (!lock) return 1;
  pthread_mutex_init(lock, NULL);

  pthread_mutex_lock(lock);
  g_data = 42;   /* line 17: protected by heap mutex */
  pthread_mutex_unlock(lock);

  int result = g_data; /* read after unlock — checker only tracks writes here */
  pthread_mutex_destroy(lock);
  free(lock);
  return result;
}
