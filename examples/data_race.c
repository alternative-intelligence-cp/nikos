#include <pthread.h>
int shared_data = 0;
pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    pthread_mutex_lock(&lock1);
    shared_data++;
    pthread_mutex_unlock(&lock1);
    return NULL;
}

void* thread2_func(void* arg) {
    // Missing lock!
    shared_data++;
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
