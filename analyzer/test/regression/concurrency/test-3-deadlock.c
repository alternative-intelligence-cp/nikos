/* test-3-deadlock.c
 * Calls pthread_mutex_lock twice on the same non-recursive mutex
 * without an intervening unlock — self-deadlock.
 * Expected: error (deadlock).
 */
#include <pthread.h>

pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
int g_val = 0;

int main() {
  pthread_mutex_lock(&g_lock);  /* line 12: first acquire — ok */
  g_val = 1;
  pthread_mutex_lock(&g_lock);  /* line 14: second acquire without unlock — deadlock */
  g_val = 2;
  pthread_mutex_unlock(&g_lock);
  pthread_mutex_unlock(&g_lock);
  return g_val;
}
