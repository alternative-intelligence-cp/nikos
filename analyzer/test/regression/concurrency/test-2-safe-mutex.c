/* test-2-safe-mutex.c
 * Global variable properly protected by a pthread mutex.
 * Expected: safe (all accesses hold the lock).
 */
#include <pthread.h>

int g_counter = 0;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg) {
  pthread_mutex_lock(&g_lock);
  g_counter++; /* line 12: protected write */
  pthread_mutex_unlock(&g_lock);
  return NULL;
}

int main() {
  pthread_t t;
  pthread_create(&t, NULL, worker, NULL);
  pthread_mutex_lock(&g_lock);
  g_counter++; /* line 21: protected write */
  pthread_mutex_unlock(&g_lock);
  pthread_join(t, NULL);
  return g_counter;
}
