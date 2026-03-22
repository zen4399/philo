#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void *thread(void *vargp);
void *printA(void *arg);
void *printB(void *arg);
void *increment(void *arg);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main() {
  pthread_t tid;
  pthread_t t1;
  pthread_t t2;
  pthread_t counter, counter2;

  pthread_create(&tid, NULL, thread, NULL);
  pthread_create(&t1, NULL, printA, NULL);
  pthread_create(&t2, NULL, printB, NULL);
  pthread_create(&counter, NULL, increment, NULL);
  pthread_create(&counter2, NULL, increment, NULL);

  pthread_join(tid, NULL);
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(counter, NULL);
  pthread_join(counter2, NULL);

  pthread_mutex_destroy(&lock);
  return 0;
}

void *thread(void *vargp) {
  printf("Hello, world!\n");
  return NULL;
}

void *printA(void *arg) {
  int i;

  i = 0;
  while (i < 5) {
    printf("A\n");
    i++;
  }
  return NULL;
}

void *printB(void *arg) {
  int i = 0;
  while (i < 5) {
    printf("B\n");
    i++;
  }
  return NULL;
}

void *increment(void *arg) {
  for (int i = 0; i < 1000000; i++) {
    pthread_mutex_lock(&lock);
    counter++;
    pthread_mutex_unlock(&lock);
  }
  printf("%d\n", counter);
  return NULL;
}
