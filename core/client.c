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
#include <sys/select.h> // select()
#include <errno.h>      // nº do último erro
#include <unistd.h>     // close()

#include "client_utils.h" // client_t, message_t

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
int client_fd, // descritor de arquivo do soquete de cliente
    msgr_fd,   //                                 de mensageiro
    ui_fd;     //                                 da interface
bool running = true;
bool isMsgr = false;

/*
 *  Assinaturas
 */
client_t setupComm(int argc, char **argv);
// módulo p/ estabelecer conexão cliente-servidor

void setupGUIComm(int port);
// módulo p/ estabelecer conexão com a interface

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (SIGINT)

void gracefulShutdown();
// rotina de encerramento gracioso

void crashLanding(char *error);
// rotina de encerramento em caso de falha

int main(int argc, char **argv){
// uso: ./client <username> <secret> <port> <gui mode>

    char error[BUFFER_SIZE];

    // configura o tratamento de sinais...
    signal(SIGINT, handleSIGINT);

    // verifica se está no modo de mensageiro
    if(argc == 5){
        if(strcmp(argv[4], "-gui") == 0){
            isMsgr = true;
        }
    }

    client_t client = setupComm(argc, argv);

    printf("Conexão estabelecida com o servidor.\n");

    if(isMsgr){
        setupGUIComm(atoi(argv[3]) + 1);

        printf("Conexão estabelecida com a interface.\n");
    }

    // lógica de comunicação
    #pragma omp parallel sections shared(running)
    {
        #pragma omp section
        {
        // entrada

            while(running){
            // recebe mensagens do servidor

                message_t message;
                resetMsg(&message);

                ssize_t rcvd = recv(client_fd,
                                    &message,
                                    sizeof(message_t),
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
                    if(message.secret == client.secret){
                        if(strcmp(message.username, client.username) != 0){
                            # pragma omp critical
                            printf("%s: %s\n", message.username, message.buffer);
                        
                            if(isMsgr){
                                ssize_t sent = send(ui_fd,
                                                    &message,
                                                    sizeof(message_t),
                                                    0);
                                if(sent < 0){ // em caso de erro, send() retorna -1
                                    FORMAT_ERROR(error, "Falha no envio da mensagem à interface: ");
                                                            
                                    crashLanding(error);
                                }
                            }
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

            message_t old_msg;
            resetMsg(&old_msg);

            char buffer[BUFFER_SIZE];
            fd_set fds; // conjunto de file descriptors

            // define o tempo máx. de espera (quando não for mensageiro)
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 250000; // 250ms

            while(running){
            // envia mensagens ao servidor

                message_t message;
                resetMsg(&message);

                if(!isMsgr){
                    FD_ZERO(&fds);              // "limpa" o conjunto de descritores de arquivo
                    FD_SET(STDIN_FILENO, &fds); // e adiciona o descritor de stdin

                    // retorna imediatamente se houver entrada ou após timeout (250ms)
                    int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout);

                    if(ready > 0){
                    // está pronto para ler a entrada padrão

                        #pragma omp critical
                        {
                            if(fgets(buffer, BUFFER_SIZE, stdin)){
                                buffer[strcspn(buffer, "\n")] = '\0'; // remove a quebra de linha, se houver
                                
                                strcpy(message.buffer, buffer);
                                strcpy(message.username, client.username);
                                message.secret = client.secret;
                                message.counter = 1; // conceitualmente, o remetente já leu a mensagem

                                if(!compareMsg(message, old_msg)){
                                // o envio da mensagem ocorre somente se a mensagem não for repetida
                                
                                    old_msg = message;

                                    ssize_t sent = send(client_fd,
                                                        &message,
                                                        sizeof(message_t),
                                                        0);
                                    if(sent < 0){ // em caso de erro, send() retorna -1
                                        FORMAT_ERROR(error, "Falha no envio da mensagem ao servidor: ");
                                                            
                                        crashLanding(error);
                                    }
                                }
                            }
                        }
                    }
                }else{
                    ssize_t rcvd = recv(ui_fd,
                                        &message,
                                        sizeof(message_t),
                                        0);
                    if(rcvd <= 0){
                        if(rcvd == 0){
                        // a conexão foi perdida

                            printf("Conexão perdida com a interface.\n");
                            
                            gracefulShutdown();
                        }else{
                            FORMAT_ERROR(error, "Falha no recebimento da mensagem da interface: ");
                                                        
                            crashLanding(error);
                        }
                    }

                    if(!compareMsg(message, old_msg)){
                    // o envio da mensagem ocorre somente se a mensagem não for repetida
                                
                        old_msg = message;

                        ssize_t sent = send(client_fd,
                                            &message,
                                            sizeof(message_t),
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

    gracefulShutdown();

    exit(EXIT_SUCCESS);
}

/*
 *  Funções
 */
client_t setupComm(int argc, char **argv){
// módulo p/ estabelecer conexão cliente-servidor

    char error[BUFFER_SIZE];

    struct sockaddr_in server_addr;
    struct sockaddr *server_addr_ptr
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    client_t client;

    // verifica os parâmetros de inicialização
    if(checkClientArgs(argc, argv) == false){
        FORMAT_ERROR(error, "Parâmetros de inicialização inválidos.\n");

        crashLanding(error);
    }
    int port = atoi(argv[3]);

    // define as informações de cliente
    strcpy(client.username, argv[1]);
    client.secret = atoi(argv[2]);

    // cria o soquete de cliente...
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

void setupGUIComm(int port){
// módulo p/ estabelecer conexão com a interface

    char error[BUFFER_SIZE];

    // Quando a GUI está ativa, a aplicação atua como um mensageiro
    struct sockaddr_in msgr_addr;
    struct sockaddr *msgr_addr_ptr
    = (struct sockaddr*)&msgr_addr;
    socklen_t msgr_addr_len = sizeof(msgr_addr);

    // cria o soquete de mensageiro (servidor local)...
    msgr_fd = socket(AF_INET,     // com protocolo IPv4 e
                     SOCK_STREAM, // baseado em conexão
                     0);
    if(msgr_fd == -1){
        FORMAT_ERROR(error, "Falha na criação do soquete de mensageiro: ");
                                
        crashLanding(error);
    }
    
    // define o endereço de mensageiro...
    msgr_addr.sin_family = AF_INET;          // para protocolo IPv4,
    msgr_addr.sin_addr.s_addr = INADDR_ANY;  // de qualquer origem e (to-do: limitar para localhost)
    msgr_addr.sin_port = htons(port);        // na porta escolhida

    // vincula...
    if(bind(msgr_fd,       // o file desciptor do soquete de mensageiro
            msgr_addr_ptr, // ao endereço de mensageiro
            msgr_addr_len) < 0){
        FORMAT_ERROR(error, "Falha de definição do endereço de mensageiro: ");
                                
        crashLanding(error);
    }

    // declara intenção de escutar novas conexões...
    if(listen(msgr_fd, // no soquete de mensageiro com
              1        // fila limite de uma requisição
             ) < 0){
        FORMAT_ERROR(error, "Falha na tentativa de conexão com a interface: ");
                                        
        crashLanding(error);
    }

    // tenta aceitar uma conexão com a interface
    while(running){
        ui_fd = accept(msgr_fd,
                       msgr_addr_ptr,
                       &msgr_addr_len);
        if(ui_fd < 0){
            sleep(1); // espera 1s antes da próx. tentativa

            continue;
        }

        break;
    }
}

void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (SIGINT)

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

    gracefulShutdown();
}