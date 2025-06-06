#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdbool.h>   // bool type
#include <sys/types.h> // pid_t
#include <sys/ipc.h>   // key_t

/*
 *  Estruturas
 */
struct message {
    char username[16];
    long secret;
    char buffer[1024];
}typedef(message_t);

struct client{
    // multiprocess.
    pid_t pid;

    // ID
    char username[16]; // 15 char + '\0'
    long secret;

    // IPC
    key_t shm_key,
          sem_key;
    int shm_id,
        sem_id;
    message_t *shm_ptr;
}typedef(client_t);

/*
 *  Assinaturas
 */
bool checkClientArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

#endif // CLIENT_UTILS_H