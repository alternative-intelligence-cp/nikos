/* test-4-conditional-lock.c
 * Lock is held on one branch of a conditional but not the other.
 * At the join point the lockset analysis computes the intersection
 * (empty), so any subsequent global access is unprotected.
 * Expected: unsafe (data race warning after the join).
 */
#include <pthread.h>

extern int __ikos_nondet_int(void);

int g_data = 0;
pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

int main() {
  if (__ikos_nondet_int()) {
    pthread_mutex_lock(&g_lock);
  }
  /* At this join point the lockset is empty (intersection of {g_lock} and {}) */
  g_data++; /* line 19: possibly unprotected */
  /* No unlock here intentionally */
  return g_data;
}
