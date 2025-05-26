#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <unistd.h>  // pid_t
#include <stdbool.h> // boolean type

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
bool checkArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

#endif // CLIENT_UTILS_H