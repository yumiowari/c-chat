/*
 *
 * Sistema de Chatting Cliente-Servidor
 *
 * Por Rafael Renó Corrêa, 2025
 * 
 * Aplicação Cliente
 * 
 */



/*
 *   Bibliotecas
 */
#include <stdlib.h> // exit()
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h> // close()
#include <arpa/inet.h> // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in

/*
 *   Definições
 */
#define PORT 8080
#define SERVER_IP "127.0.0.1"

/*
 *   Variáveis Globais
 */

/*
 *   Assinaturas
 */

 int main(int argc, char **argv){
    int client_fd;
    struct sockaddr_in server_addr;   // endereço do servidor, especialmente para IPv4
        struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
        = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    // cria o soquete do cliente...
    client_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(client_fd == -1){
        exit(EXIT_FAILURE);
    }

    // define o endereço do servidor...
    server_addr.sin_family = AF_INET;   // para protocolo IPv4,
    if(inet_pton(AF_INET,
                 SERVER_IP,             // no endereço IP localhost
                 &server_addr.sin_addr
                ) <= 0){
        exit(EXIT_FAILURE);
    }
    server_addr.sin_port = htons(PORT); // e porta 8080

    // estabelece conexão com o servidor...
    if(connect(client_fd,
               server_addr_ptr,
               server_addr_len) < 0){
        exit(EXIT_FAILURE);
    }

    printf("Conexão estabelecida com o servidor.\n");

    exit(EXIT_SUCCESS);
}

/*
 *   Funções
 */