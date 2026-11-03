#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int number;
} Event;

pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t my_cond = PTHREAD_COND_INITIALIZER;

int ready = 0;
Event* shared_data = NULL;

void* producer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        sleep(1);

        pthread_mutex_lock(&my_mutex);

        Event* new_event = (Event*)malloc(sizeof(Event));
        new_event->number = i * 10;

        shared_data = new_event;
        ready = 1;

        printf("[Поставщик]: Отправлено событие со значением %d\n", new_event->number);

        pthread_cond_signal(&my_cond);
        pthread_mutex_unlock(&my_mutex);
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 1; i <= 5; i++) {
        pthread_mutex_lock(&my_mutex);

        while (ready == 0) {
            pthread_cond_wait(&my_cond, &my_mutex);
        }

        Event* received_event = shared_data;

        shared_data = NULL;
        ready = 0;

        printf("[Потребитель]: Обработано событие. Данные внутри: %d\n", received_event->number);

        free(received_event);

        pthread_mutex_unlock(&my_mutex);
    }
    return NULL;
}

int main() {
    pthread_t th_producer, th_consumer;

    pthread_create(&th_producer, NULL, producer, NULL);
    pthread_create(&th_consumer, NULL, consumer, NULL);

    pthread_join(th_producer, NULL);
    pthread_join(th_consumer, NULL);

    pthread_mutex_destroy(&my_mutex);
    pthread_cond_destroy(&my_cond);

    return 0;
}
