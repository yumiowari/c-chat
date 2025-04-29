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
#include <stdio.h>      // I/O
#include <stdbool.h>    // bool typedef
#include <unistd.h>     // close()
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // strcmp()
#include <signal.h>     // signal()
#include <stdatomic.h>  // atomic_bool typedef
#include <sys/select.h> // select()
#include <errno.h>      // nº do último erro

/*
 *   Definições
 */
#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 1

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

void gracefulShutdown();
// rotina de encerramento gracioso

void crashLanding(char *e);
// rotina de encerramento em caso de falha

int main(int argc, char **argv){
    struct sockaddr_in server_addr;   // endereço do servidor, especialmente para IPv4
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char error[1024];

    // configura o tratamento de sinais...
    signal(SIGINT, handleSIGINT);

    // cria o soquete do cliente...
    client_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(client_fd == -1){
        strcat(error, "Falha na criação do soquete de cliente: ");
        strcat(error, strerror(errno));

        crashLanding(error);
    }

    // define o endereço do servidor...
    server_addr.sin_family = AF_INET;   // para protocolo IPv4,
    if(inet_pton(AF_INET,
                 SERVER_IP,             // no endereço IP localhost
                 &server_addr.sin_addr
                ) <= 0){
        strcpy(error, "Falha na definição do endereço do servidor: ");
        strcat(error, strerror(errno));

        crashLanding(error);
    }
    server_addr.sin_port = htons(PORT); // e porta 8080

    // estabelece conexão com o servidor...
    if(connect(client_fd,
               server_addr_ptr,
               server_addr_len) < 0){
        strcpy(error, "Falha na tentativa de conexão com o servidor: ");
        strcat(error, strerror(errno));
                
        crashLanding(error);
    }

    printf("Conexão estabelecida com o servidor.\n");

    // lógica de comunicação
    #pragma omp parallel sections
    {
        #pragma omp section
        {
        // entrada
        
            char buffer[BUFFER_SIZE];

            while(running){
                ssize_t rcvd = recv(client_fd,
                                    buffer,
                                    BUFFER_SIZE,
                                0);

                if(rcvd <= 0){
                    if(rcvd == 0){
                    // conexão perdida

                        printf("Conexão perdida com o servidor.\n");

                        break;
                    }else{
                        strcpy(error, "Falha no recebimento de mensagem do servidor: ");
                        strcat(error, strerror(errno));
                                
                        crashLanding(error);
                    }
                }
            }
        }

        #pragma omp section
        {
        // saída

            char buffer[BUFFER_SIZE];
            struct timeval timeout;

            while(running){
                fd_set fds;

                FD_ZERO(&fds);
                FD_SET(STDIN_FILENO, &fds); // atribui stdin ao conjunto de file descriptors
                
                timeout.tv_sec = TIMEOUT_SEC;
                timeout.tv_usec = 0;

                int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);
                
                if(ready > 0){
                    #pragma omp critical
                    scanf("%s", buffer);

                    ssize_t sent = send(client_fd,
                                        buffer,
                                        strlen(buffer),
                                        0);
                    if(sent < 0){ // em caso de erro, send() retorna -1
                        strcpy(error, "Falha no envio de mensagem ao servidor: ");
                        strcat(error, strerror(errno));
                                
                        crashLanding(error);
                    }
                }else if(ready == 0){
                    continue;
                }else{
                    strcpy(error, "Falha na verificação da entrada padrão: ");
                    strcat(error, strerror(errno));
                                
                    crashLanding(error);
                }
            }
        }
    }

    gracefulShutdown();

    exit(EXIT_SUCCESS);
}

/*
 *   Funções
 */
void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    gracefulShutdown();

    exit(EXIT_SUCCESS);
}

void gracefulShutdown(){
// rotina de encerramento gracioso

    close(client_fd);

    running = false; // encerra os laços de repetição

    exit(EXIT_SUCCESS);
}

void crashLanding(char *e){
// rotina de encerramento em caso de falha

    fprintf(stderr, strcat(e, "\n"));

    fprintf(stderr, "Fim abrupto da aplicação.\n");

    close(client_fd);

    running = false;

    exit(EXIT_FAILURE);
}