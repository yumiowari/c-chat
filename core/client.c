/*
 *
 *  Sistema de Chatting Cliente-Servidor
 *
 *  Copyright © 2025 Rafael Renó Corrêa | owariyumi@gmail.com
 * 
 *  Todos os direitos reservados.
 * 
 *  APLICAÇÃO CLIENTE
 * 
 */

/*
 *  Bibliotecas
 */
#include <stdlib.h>     // exit()
#include <stdio.h>      // I/O
#include <stdbool.h>    // bool type
#include <sys/types.h>  // pid_t
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // strcmp()
#include <signal.h>     // signal()
#include <stdatomic.h>  // atomic_bool typedef
#include <sys/select.h> // select()
#include <errno.h>      // nº do último erro
#include <unistd.h>     // close()

#include "client_utils.h" // struct client
#include "comm_utils.h"   // wrap(), unwrap()

/*
 *  Definições
 */
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 1

/*
 *  Macros
 */
#define FORMAT_ERROR(error, prefix) strcpy(error, prefix); if(errno != 0)strcat(error, strerror(errno));

/*
 *  Variáveis Globais
 */
int client_fd;
bool running = true;

/*
 *  Assinaturas
 */
struct client setupComm(int argc, char **argv);
// módulo p/ estabelecer conexão cliente-servidor

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void gracefulShutdown();
// rotina de encerramento gracioso

void crashLanding(char *error);
// rotina de encerramento em caso de falha

int main(int argc, char **argv){
    char error[BUFFER_SIZE];

    // configura o tratamento de sinais...
    signal(SIGINT, handleSIGINT);

    struct client client = setupComm(argc, argv);

    printf("Conexão estabelecida com o servidor.\n");

    // lógica de comunicação
    #pragma omp parallel sections shared(running)
    {
        #pragma omp section
        {
        // entrada

            char buffer[BUFFER_SIZE];
            char username[16];
            long secret;
            char message[BUFFER_SIZE];

            while(running){
            // recebe mensagens do servidor

                ssize_t rcvd = recv(client_fd,
                                    buffer,
                                    BUFFER_SIZE,
                                    0);
                if(rcvd <= 0){
                    if(rcvd == 0){
                    // conexão perdida

                        printf("Conexão perdida com o servidor.\n");

                        gracefulShutdown();
                    }else{
                        FORMAT_ERROR(error, "Falha no recebimento da mensagem do servidor: ");
                                
                        crashLanding(error);
                    }
                }else{
                    buffer[rcvd] = '\0';

                    unwrap(buffer, username, &secret, message);

                    if(secret == client.secret){
                        if(strcmp(username, client.username) != 0){
                            # pragma omp critical
                            printf("%s: %s\n", username, message);
                        }
                    }else{
                        FORMAT_ERROR(error, "Não foi possível validar a mensagem.\n"
                                            "A conexão é insegura.\n");

                        crashLanding(error);
                    }
                }
            }
        }

        #pragma omp section
        {
        // saída

            char buffer[BUFFER_SIZE];
            char message[BUFFER_SIZE];
            fd_set fds; // conjunto de file descriptors

            // define o tempo máx. de espera
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 250000; // 250ms

            while(running){
            // envia mensagens ao servidor

                FD_ZERO(&fds);              // "limpa" o conjunto de descritores de arquivo
                FD_SET(STDIN_FILENO, &fds); // e adiciona o descritor de stdin

                // retorna imediatamente se houver entrada ou após timeout (250ms)
                int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);

                if(ready > 0){

                    #pragma omp critical
                    {
                        if(fgets(message, BUFFER_SIZE, stdin)){
                            message[strcspn(message, "\n")] = '\0'; // remove a quebra de linha, se houver
                            
                            wrap(buffer, client.username, client.secret, message);

                            ssize_t sent = send(client_fd,
                                                buffer,
                                                strlen(buffer),
                                                0);
                            if(sent < 0){ // em caso de erro, send() retorna -1
                                FORMAT_ERROR(error, "Falha no envio da mensagem ao servidor: ");
                                                    
                                crashLanding(error);
                            }
                        }
                    }
                }
            }
        }
    }

    gracefulShutdown();

    exit(EXIT_SUCCESS);
}

/*
 *  Funções
 */
struct client setupComm(int argc, char **argv){
// módulo p/ estabelecer conexão cliente-servidor

    struct sockaddr_in server_addr;   // endereço do servidor
    struct sockaddr *server_addr_ptr  // ponteiro genérico p/ o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char error[BUFFER_SIZE];
    struct client client;
    int port;

    // verifica os parâmetros de inicialização
    if(checkClientArgs(argc, argv) == false){
        FORMAT_ERROR(error, "Parâmetros de inicialização inválidos.\n");

        crashLanding(error);
    }
    port = atoi(argv[3]);

    // define as informações de cliente
    strcpy(client.username, argv[1]);
    client.secret = atoi(argv[2]);

    // cria o soquete do cliente...
    client_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(client_fd == -1){
        FORMAT_ERROR(error, "Falha na criação do soquete de cliente: ");

        crashLanding(error);
    }

    // define o endereço do servidor...
    server_addr.sin_family = AF_INET;   // para protocolo IPv4,
    if(inet_pton(AF_INET,
                 SERVER_IP,             // no endereço IP localhost
                 &server_addr.sin_addr
                ) <= 0){
        FORMAT_ERROR(error, "Falha na definição do endereço do servidor: ");

        crashLanding(error);
    }
    server_addr.sin_port = htons(port);

    // estabelece conexão com o servidor...
    if(connect(client_fd,
               server_addr_ptr,
               server_addr_len) < 0){
        FORMAT_ERROR(error, "Falha na tentativa de conexão com o servidor: ");
                
        crashLanding(error);
    }

    // informa os dados de cliente para o servidor
    ssize_t sent = send(client_fd,
                        &client,
                        sizeof(client),
                        0);
    if(sent < 0){ // em caso de erro, send() retorna -1
        FORMAT_ERROR(error, "Falha ao informar os dados de cliente ao servidor: ");
                                                    
        crashLanding(error);
    }

    return client;
}

void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    gracefulShutdown();

    exit(EXIT_SUCCESS);
}

void gracefulShutdown(){
// rotina de encerramento gracioso

    running = false;
    close(client_fd);

    exit(EXIT_SUCCESS);
}

void crashLanding(char *error){
// rotina de encerramento em caso de falha

    fprintf(stderr, "%s\n", error);
    fprintf(stderr, "Fim abrupto da aplicação.\n");

    running = false;
    close(client_fd);

    exit(EXIT_FAILURE);
}