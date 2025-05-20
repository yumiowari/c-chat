#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdlib.h> // pid_t

/*
 *  Estruturas
 */
struct client_info{
    pid_t pid;
    char username[16]; // 15 char + '\0'
    long secret;
};

/*
 *  Assinaturas
 */
long hashing(char username[16]);
// função p/ converter o nome de usuário para um código hash

#endif // CLIENT_UTILS_H