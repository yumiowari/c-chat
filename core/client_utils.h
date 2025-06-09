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
    int counter; // quando -1, indica que a mensagem é inválida
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
// função p/ verificar os parâmetros de inicialização

bool compareMsg(message_t A, message_t B);
// função p/ verificar se A == B
// retorna...
//     true - se as mensagens forem iguais
//     false - se as mensagens forem diferentes

void debugMsg(message_t msg);
// função p/ imprimir os atributos da mensagem

void resetMsg(message_t *msg);
// função p/ resetar os atributos da mensagem

#endif // CLIENT_UTILS_H