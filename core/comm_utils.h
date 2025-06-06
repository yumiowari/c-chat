#ifndef COMM_UTILS_H
#define COMM_UTILS_H

/*
 *  Bibliotecas
 */
#include <stdbool.h>

/*
 *  Estruturas
 */
union semun {
    int             val;
    struct semid_ds *buf;
    unsigned short  *array;
    struct seminfo  *__buf;
};

/*
 *  Assinaturas
 */
bool sem_wait(int sem_id);
// função p/ esperar o semáforo abrir

bool sem_signal(int sem_id);
// função p/ liberar o semáforo

#endif // COMM_UTILS_H