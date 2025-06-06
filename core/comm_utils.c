/*
 *  Bibliotecas
 */
#include <stdlib.h>  // strtol()
#include <stdio.h>   // sprintf()
#include <string.h>  // strcpy(), strcat()
#include <stdbool.h> // bool typedef
#include <sys/sem.h> // semaphore

#include "comm_utils.h"

/*
 *  Funções
 */
bool sem_wait(int sem_id){
// função p/ esperar o semáforo abrir

    // operação P: decrementa o valor do semáforo.
    // se ele for zero ou menor, o processo bloqueia e espera.
    struct sembuf op = {0, -1, 0};
    if(semop(sem_id, &op, 1) == 1)
        return false;
    else
        return true;
}

bool sem_signal(int sem_id){
// função p/ liberar o semáforo
    
    // operação V: incrementa o valor do semáforo.
    // se algum processo estiver bloqueado esperando,
    // ele será desbloqueado.
    struct sembuf op = {0, 1, 0};
    if(semop(sem_id, &op, 1) == 1)
        return false;
    else
        return true;
}