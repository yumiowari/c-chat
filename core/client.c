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
#include <stdlib.h>     // exit()
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>     // close()
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // strcmp()
#include <signal.h>
#include <stdatomic.h>

/*
 *   Definições
 */
#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

/*
 *   Variáveis Globais
 */
int client_fd;
volatile atomic_bool running = true;

/*
 *   Assinaturas
 */
void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

int main(int argc, char **argv){
    struct sockaddr_in server_addr;   // endereço do servidor, especialmente para IPv4
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    // configura o tratamento de sinais...
    signal(SIGINT, handleSIGINT);

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

    // lógica de comunicação
    #pragma omp parallel sections
    {
        #pragma omp section
        {
        // entrada
            
        }

        #pragma omp section
        {
        // saída

            char buffer[BUFFER_SIZE];

            while(running){
                #pragma omp critical
                scanf("%s", buffer);

                ssize_t sent = send(client_fd,
                                    buffer,
                                    strlen(buffer),
                                    0);
                if(sent < 0) // em caso de erro, send() retorna -1
                    exit(EXIT_FAILURE);
            }
        }
    }

    close(client_fd); // encerra a conexão e notifica o servidor

    exit(EXIT_SUCCESS);
}

/*
 *   Funções
 */
void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    close(client_fd);

    running = false; // encerra os laços de repetição

    exit(EXIT_SUCCESS);
}