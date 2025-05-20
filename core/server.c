/*
 *
 *  Sistema de Chatting Cliente-Servidor
 *
 *  Copyright © 2025 Rafael Renó Corrêa | owariyumi@gmail.com
 * 
 *  Todos os direitos reservados.
 * 
 *  APLICAÇÃO SERVIDORA
 * 
 */

/*
 *  Bibliotecas
 */
#include <stdlib.h>     // multiprocessing, exit()
#include <stdio.h>      // I/O
#include <stdbool.h>    // bool typedef
#include <unistd.h>     // close()
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // memset()
#include <signal.h>     // signal()
#include <wait.h>       // waitpid()
#include <stdatomic.h>  // atomic_bool typedef
#include <errno.h>      // nº do último erro

#include "client_utils.h"

/*
 *  Definições
 */
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CHILDREN 1024

/*
 *  Variáveis Globais
 */
int server_fd,
    client_fd;
volatile atomic_bool running = true;
struct client_info children[MAX_CHILDREN]; // array de processos filhos (clientes)
int children_qty = 0;                      // contador de filhos (solução temporária)

/*
 *  Assinaturas
 */
void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

void handleSIGTERM(int signal);
// função p/ tratar o sinal de encerramento de processo

void gracefulShutdown(int context);
// rotina de encerramento gracioso

void crashLanding(int context, char *e);
// rotina de encerramento em caso de falha

void killOffspring();
// função p/ matar os processos filhos

int main(int argc, char **argv){
    struct sockaddr_in server_addr;   // endereço do servidor, especialmente para IPv4
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char error[1024];

    // configura o tratamento de sinais...
    signal(SIGINT,  handleSIGINT);
    signal(SIGCHLD, handleSIGCHLD);

    // cria o soquete de servidor...
    server_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(server_fd == -1){
        strcpy(error, "Falha na criação do soquete de servidor: ");
        strcat(error, strerror(errno));
                                
        crashLanding(0, error);
    }

    // define o endereço de servidor...
    server_addr.sin_family = AF_INET;         // para protocolo IPv4,
    server_addr.sin_addr.s_addr = INADDR_ANY; // de qualquer origem
    server_addr.sin_port = htons(PORT);       // e porta 8080

    // vincula...
    if(bind(server_fd,       // o file desciptor do soquete do servidor
            server_addr_ptr, // ao endereço do servidor
            server_addr_len) < 0){
        strcpy(error, "Falha de definição do endereço de servidor: ");
        strcat(error, strerror(errno));
                                
        crashLanding(0, error);
    }

    // declara intenção de escutar novas conexões...
    if(listen(server_fd, // no soquete do servidor com
              5          // fila limite de 5 requisições
             ) < 0){
        strcpy(error, "Falha na tentativa de conexão com o cliente: ");
        strcat(error, strerror(errno));
                                        
        crashLanding(0, error);
    }

    printf("Servidor on-line e ouvindo na porta %d!\n", PORT);

    while(running == true){
    // loop do servidor

        // aceita uma nova conexão...
        client_fd = accept(server_fd,
                           server_addr_ptr,
                           &server_addr_len);
        if(client_fd < 0){
            sleep(1);

            continue;
        }

        // recebe os dados do cliente
        struct client_info client;
        ssize_t rcvd = recv(client_fd,
                            &client,
                            sizeof(client),
                            0);
        if(rcvd <= 0){
            if(rcvd == 0){
            // conexão perdida

                printf("Conexão perdida com o cliente.\n");
                            
                gracefulShutdown(1);
            }else{
                strcpy(error, "Falha ao receber os dados do cliente: ");
                strcat(error, strerror(errno));
                                                        
                crashLanding(1, error);
            }
        }

        printf("Conexão estabelecida com %s!\n", client.username);

        pid_t pid = fork();

        if(pid < 0){
        // fork() falhou
        
            strcpy(error, "Falha na criação do processo para comunicação com o cliente: ");
            strcat(error, strerror(errno));
                                    
            crashLanding(0, error);
        }else if(pid == 0){
        // processo filho
        
            close(server_fd); // não aceita novas conexões

            // configura tratamento de sinais
            signal(SIGINT,  SIG_IGN);
            signal(SIGTERM, handleSIGTERM);
            
            // lógica de comunicação
            #pragma omp parallel sections
            {
                #pragma omp section
                {
                // entrada

                    char buffer[BUFFER_SIZE];

                    while(running == true){

                        ssize_t rcvd = recv(client_fd,
                                            buffer,
                                            BUFFER_SIZE,
                                            0);
                        if(rcvd <= 0){
                            if(rcvd == 0){
                            // conexão perdida

                                printf("Conexão perdida com o cliente.\n");
                            
                                gracefulShutdown(1);
                            }else{
                                strcpy(error, "Falha no recebimento de mensagem do cliente: ");
                                strcat(error, strerror(errno));
                                                        
                                crashLanding(1, error);
                            }
                        }
                            
                        buffer[rcvd] = '\0';

                        #pragma omp critical
                        printf("(%d) %s\n", getpid(), buffer);

                        memset(buffer, 0, BUFFER_SIZE);
                    }
                }

                #pragma omp section
                {
                // saída

                    while(running == true){

                        ssize_t sent = send(client_fd,
                                            &client.secret,
                                            sizeof(client.secret),
                                            0);
                        if(sent < 0){ // em caso de erro, send() retorna -1
                            strcpy(error, "Falha no envio de mensagem ao cliente: ");
                            strcat(error, strerror(errno));
                                                    
                            crashLanding(1, error);
                        }else{
                            sleep(1);
                        }
                    }
                }
            }

            close(client_fd);
        }else{
        // processo pai

            close(client_fd); // prepara para estabelecer novas conexões

            // insere o cliente no array de processos filhos
            client.pid = pid;
            children[children_qty] = client;

            children_qty++;
        }
    }

    exit(EXIT_SUCCESS);
}

/*
 *  Funções
 */
void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    gracefulShutdown(0);

    exit(EXIT_SUCCESS);
}

void handleSIGCHLD(int signal){
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("\nProcesso filho com PID %d terminou.\n", pid);

        for(int i = 0; i < children_qty; i++){ // O(n * m)
            if(children[i].pid == pid){
                for(int j = i; j < children_qty - 1; j++)
                    children[j] = children[j + 1];

                children_qty--;

                break;
            }
        }
    }
}

void handleSIGTERM(int signal){
// função p/ tratar o sinal de encerramento de processo

    printf("\n(%d) Sinal de término recebido.\n"
           "Encerrando processo filho...\n", getpid()); // no contexto do processo filho

    gracefulShutdown(0);

    exit(EXIT_SUCCESS);
}

void gracefulShutdown(int context){
// rotina de encerramento gracioso

    switch(context){
        case 0: // processo pai

            running = false; // encerra os laços de repetição

            killOffspring();

            close(server_fd);

            break;

        case 1: // processo filho

            running = false;

            close(client_fd);

            break;
    }
    
    exit(EXIT_SUCCESS);
}
    
void crashLanding(int context, char *e){
// rotina de encerramento em caso de falha

    switch(context){
        case 0: // processo pai

            fprintf(stderr, "%s\n", e);

            fprintf(stderr, "Fim abrupto da aplicação.\n");

            running = false;

            killOffspring();

            close(server_fd);

            break;

        case 1: // processo filho

            fprintf(stderr, "%s\n", e);

            fprintf(stderr, "Fim abrupto do processo %d.\n", getpid());

            running = false;

            close(client_fd);

            break;
    }
    
    exit(EXIT_FAILURE);
}

void killOffspring(){
// função p/ matar os processos filhos

    for(int i = 0; i < children_qty; i++){
        if(kill(children[i].pid, SIGTERM) == -1){
            fprintf(stderr, "Falha ao enviar SIGTERM para o processo filho %d: %s\n", children[i], strerror(errno));
        }else{
            printf("Encerrando processo filho %d...\n", children[i]);
        }
    }
}