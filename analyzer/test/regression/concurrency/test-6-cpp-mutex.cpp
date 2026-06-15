/* test-6-cpp-mutex.cpp
 * C++ std::mutex protecting a global variable.
 * std::mutex::lock() maps to LibcPthreadMutexLock via 12-A.
 * Expected: safe.
 */
#include <mutex>
#include <pthread.h>

static int g_shared = 0;
static std::mutex g_mutex;

extern "C" void* worker(void* arg) {
  g_mutex.lock();
  g_shared++; /* protected write */
  g_mutex.unlock();
  return NULL;
}

int main() {
  pthread_t t;
  pthread_create(&t, NULL, worker, NULL);
  g_mutex.lock();
  g_shared++; /* protected write */
  g_mutex.unlock();
  pthread_join(t, NULL);
  return g_shared;
}
