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
#include <stdbool.h>    // boolean type
#include <unistd.h>     // typedefs
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // memset()
#include <signal.h>     // signal()
#include <wait.h>       // waitpid()
#include <stdatomic.h>  // atomic_bool typedef
#include <errno.h>      // nº do último erro

#include "server_utils.h"
#include "client_utils.h"
#include "comm_utils.h"

/*
 *  Definições
 */
#define BUFFER_SIZE 1024
#define MAX_CHILDREN 1024

/*
 *  Macros
 */
#define FORMAT_ERROR(error, prefix)     \
    do{                                 \
        strcpy(error, prefix);          \
        strcat(error, strerror(errno)); \
    }while(0)

/*
 *  Variáveis Globais
 */
int server_fd,
    client_fd;
bool running = true;
struct client children[MAX_CHILDREN]; // array de processos filhos (clientes)
int children_qty = 0;                 // contador de filhos (solução temporária)

/*
 *  Assinaturas
 */
struct server setupComm(int argc, char **argv);
// módulo p/ estabelecer conexão cliente-servidor

struct client tryAccept(struct server server);
// tentar aceitar uma nova conexão com um cliente

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
    char error[BUFFER_SIZE];

    // configura o tratamento de sinais...
    signal(SIGINT,  handleSIGINT);
    signal(SIGCHLD, handleSIGCHLD);

    struct server server = setupComm(argc, argv);

    printf("Servidor on-line e ouvindo na porta %d!\n", server.port);

    while(running == true){
    // loop do servidor

        struct client client = tryAccept(server);

        printf("Conexão estabelecida com %s!\n", client.username);

        pid_t pid = fork();

        if(pid < 0){
        // fork() falhou
        
            FORMAT_ERROR(error, "Falha na criação do processo para comunicação com o cliente: ");
                                    
            crashLanding(0, error);
        }else if(pid == 0){
        // processo filho
        
            close(server_fd); // não aceita novas conexões

            // configura tratamento de sinais
            signal(SIGINT,  SIG_IGN);
            signal(SIGTERM, handleSIGTERM);
            
            // lógica de comunicação
            #pragma omp parallel sections shared(running)
            {
                #pragma omp section
                {
                // entrada

                    char buffer[BUFFER_SIZE];

                    while(running == true){
                    // recebe mensagens do cliente e encaminha para os outros membros do grupo

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
                                FORMAT_ERROR(error, "Falha no recebimento da mensagem do cliente: ");
                                                        
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
                    // lê mensagens dos outros membros do grupo e encaminha para o cliente

                        ssize_t sent = send(client_fd,
                                            &client.secret,
                                            sizeof(client.secret),
                                            0);
                        if(sent < 0){ // em caso de erro, send() retorna -1
                            FORMAT_ERROR(error, "Falha no envio da mensagem ao cliente: ");
                                                    
                            crashLanding(1, error);
                        }else{
                            sleep(1); // espera 1 segundo antes da próxima mensagem
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
struct server setupComm(int argc, char **argv){
// módulo p/ estabelecer conexão cliente-servidor

    struct sockaddr_in server_addr;   // endereço do servidor
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char error[BUFFER_SIZE];
    struct server server;

    // verifica os parâmetros de inicialização
    if(checkServerArgs(argc, argv) == false){
        FORMAT_ERROR(error, "Parâmetros de inicialização inválidos.\n");

        crashLanding(0, error);
    }

    server.port = atoi(argv[1]);

    // cria o soquete de servidor...
    server_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(server_fd == -1){
        FORMAT_ERROR(error, "Falha na criação do soquete de servidor: ");
                                
        crashLanding(0, error);
    }

    // define o endereço de servidor...
    server_addr.sin_family = AF_INET;          // para protocolo IPv4,
    server_addr.sin_addr.s_addr = INADDR_ANY;  // de qualquer origem
    server_addr.sin_port = htons(server.port);

    // vincula...
    if(bind(server_fd,       // o file desciptor do soquete do servidor
            server_addr_ptr, // ao endereço do servidor
            server_addr_len) < 0){
        FORMAT_ERROR(error, "Falha de definição do endereço de servidor: ");
                                
        crashLanding(0, error);
    }

    // declara intenção de escutar novas conexões...
    if(listen(server_fd, // no soquete do servidor com
              5          // fila limite de 5 requisições
             ) < 0){
        FORMAT_ERROR(error, "Falha na tentativa de conexão com o cliente: ");
                                        
        crashLanding(0, error);
    }

    server.server_addr = server_addr;
    server.server_addr_ptr = server_addr_ptr;
    server.server_addr_len = server_addr_len;

    return server;
}

struct client tryAccept(struct server server){
// tentar aceitar uma nova conexão com um cliente

    char error[BUFFER_SIZE];

    while(running){
        client_fd = accept(server_fd,
                           server.server_addr_ptr,
                           &server.server_addr_len);
        if(client_fd < 0){
            sleep(1);

            continue;
        }

        // recebe os dados do cliente
        struct client client;
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
                FORMAT_ERROR(error, "Falha na recepção das informações do cliente: ");
                                                        
                crashLanding(1, error);
            }
        }

        return client;
    }
}

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

    gracefulShutdown(1);

    exit(EXIT_SUCCESS);
}

void gracefulShutdown(int context){
// rotina de encerramento gracioso

    switch(context){
        case 0:
        // processo pai

            running = false; // encerra os laços de repetição
            killOffspring();
            close(server_fd);

            break;

        case 1:
        // processo filho

            running = false;
            close(client_fd);

            break;
    }
    
    exit(EXIT_SUCCESS);
}
    
void crashLanding(int context, char *error){
// rotina de encerramento em caso de falha

    switch(context){
        case 0:
        // processo pai

            fprintf(stderr, "%s\n", error);
            fprintf(stderr, "Fim abrupto da aplicação.\n");

            running = false;
            killOffspring();
            close(server_fd);

            break;

        case 1:
        // processo filho

            fprintf(stderr, "%s\n", error);
            fprintf(stderr, "Fim abrupto do processo %d.\n", getpid());

            running = false;
            close(client_fd);

            break;
    }
    
    exit(EXIT_FAILURE);
}

void killOffspring(){
// função p/ matar os processos filhos

    char error[BUFFER_SIZE];

    for(int i = 0; i < children_qty; i++){
        if(kill(children[i].pid, SIGTERM) == -1){
            FORMAT_ERROR(error, "Falha ao enviar SIGTERM para o processo filho: ");

            crashLanding(0, error);
        }else{
            printf("Encerrando processo filho %d...\n", children[i].pid);
        }
    }
}