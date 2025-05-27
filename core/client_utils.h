#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <sys/types.h> // pid_t
#include <stdbool.h>   // bool type

/*
 *  Estruturas
 */
struct client{
    pid_t pid;
    char username[16]; // 15 char + '\0'
    long secret;
    key_t shm_key;
    int shm_id;
    key_t sem_key;
    int sem_id;
    char *shm_ptr;
};

/*
 *  Assinaturas
 */
bool checkClientArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

#endif // CLIENT_UTILS_H