#include <stdio.h>
#include <stdlib.h>
#include "semaphore.h"
#include <pthread.h>

int counter = 0;
int buffLen = 5;
int buff[5];
int curr = 0;
int valid = -1;

sem_t buffItems;
sem_t buffEmptySpaces;
sem_t counters;


int buffIsFull() {
    return curr == buffLen && valid == -1
           || curr == valid - 1;
}


int buffIsEmpty() {
    return !(curr - valid + 1);
}

void addToBuff(int item) {
    buff[curr++] = item;
    curr %= buffLen;
}

int getBuffItem() {
    int item =  buff[++valid];
    if (valid == buffLen - 1)
        valid = -1;

    return item;
}

void *countersTask(void *p) {
    fprintf(stdout, "Counter thread %d: received a message", *(int *)p);
    fprintf(stdout, "Counter thread %d: waiting to write", *(int *)p);
    sem_wait(&counters);
    counter++;
    fprintf(stdout, "Counter thread %d: now adding to counter, counter value=%d", *(int *)p, counter);
    sem_post(&counters);
}

void *monitorTask(void *p) {
    sem_wait(&counters);
    if (buffIsFull())
        fprintf(stdout, "Monitor thread: Buffer full!!");
    sem_post(&buffItems);

    sem_wait(&buffEmptySpaces);
    fprintf(stdout, "Monitor thread: waiting to read counter");
    sem_wait(&counters);
    fprintf(stdout, "Monitor thread: reading a counter value of %d", counter);
    addToBuff(counter);
    fprintf(stdout, "Monitor thread: writing to buffer at position %d", curr - 1);
    counter = 0;
    sem_post(&buffItems);
    sem_post(&counters);
}

void *collectorTask() {
    sem_wait(&counters);
    if (buffIsEmpty())
        fprintf(stdout, "Collector thread: nothing is in the buffer!");
    sem_post(&buffItems);

    sem_wait(&buffItems);
    sem_wait(&counters);
    getBuffItem();
    fprintf(stdout, "Collector thread: reading from the buffer at position %d", valid);
    sem_post(&buffEmptySpaces);
    sem_post(&counters);
}

void
int main() {
    int N = 10;
    pthread_t mCounter[N];
    int countersParam[N];
    pthread_t mMonitor;
    pthread_t mCollector;
    sem_init(&buffItems, 0, 0);
    sem_init(&buffEmptySpaces, 0, (unsigned) buffLen);
    sem_init(&counters, 0, 1);

    for (int i = 0; i < N; i++) {
        countersParam[i] = i + 1;
        pthread_create(mCounter + i, NULL, countersTask, countersParam + i);
    }

    pthread_create(&mMonitor, NULL, monitorTask, NULL);
    pthread_create(&mCollector, NULL, collectorTask, NULL);

    if (pthread_join(mMonitor, NULL)) {
        fprintf(stderr, "Some thing went wrong.");
        exit(-1);
    }

    return 0;
}