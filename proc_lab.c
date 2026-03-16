// proc_lab.c
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* worker(void* arg) {
    while (1) sleep(1);
}

int main() {
    pthread_t t;
    pthread_create(&t, NULL, worker, NULL);
    while (1) sleep(1);
}
