#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

/*
 *  Bibliotecas
 */
#include <arpa/inet.h> // struct sockaddr

/*
 *  Estruturas
 */
struct server{
    struct sockaddr_in server_addr;   // endereço do servidor
    struct sockaddr *server_addr_ptr; // ponteiro genérico para o endereço do servidor
    socklen_t server_addr_len;
    int port; // porta
}typedef(server_t);

/*
 *  Assinaturas
 */
bool checkServerArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

#endif // SERVER_UTILS_H