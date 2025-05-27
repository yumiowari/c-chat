#ifndef COMM_UTILS_H
#define COMM_UTILS_H

#include <stdbool.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

void wrap(char *buffer, char *username, long secret, char *message);
// função p/ encapsular a mensagem

void unwrap(char *buffer, char *username, long *secret, char *message);
// função p/ desencapsular a mensagem

bool sem_wait(int sem_id);
// função p/ esperar o semáforo abrir

bool sem_signal(int sem_id);
// função p/ liberar o semáforo

#endif // COMM_UTILS_H