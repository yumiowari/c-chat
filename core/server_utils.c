/*
 *  Bibliotecas
 */
#include <stdio.h>   // fprintf()
#include <stdbool.h> // bool type
#include <string.h>  // strlen()
#include <sys/sem.h> // semaphore

#include "server_utils.h"

/*
 *  Funções
 */
bool checkServerArgs(int argc, char **argv){
// função p/ verificar os parâmetros de inicialização
    
    bool flag = true;

    if(argc == 2){
        for(int i = 0; i < strlen(argv[1]); i++){
            if(argv[1][i] < '0' || argv[1][i] > '9'){
                fprintf(stderr, "A porta deve ser um número inteiro.\n");

                flag = false;

                break;
            }
        }
    }else{
        fprintf(stderr, "Parâmetros inválidos.\n"
                        "Uso: ./server <porta>\n");

        flag = false;
    }

    return flag;
}

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

bool sem_open(int sem_id){
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