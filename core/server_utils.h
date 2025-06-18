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
    socklen_t server_addr_len;        // tamanho do endereço do servidor
    int port;                         // porta
}typedef(server_t);

union semun{
    int             val;    // valor para SETVAL
    struct semid_ds *buf;   // buffer para IPC_STAT, IPC_SET
    unsigned short  *array; // array para GETALL, SETALL
    struct seminfo  *__buf; // buffer para IPC_INFO (linux-specific)
};

/*
 *  Assinaturas
 */
bool checkServerArgs(int argc, char **argv);
// função p/ verificar os parâmetros de inicialização

bool sem_wait(int sem_id);
// função p/ esperar o semáforo abrir

bool sem_open(int sem_id);
// função p/ liberar o semáforo

#endif // SERVER_UTILS_H