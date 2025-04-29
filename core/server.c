/*
 *
 * Sistema de Chatting Cliente-Servidor
 *
 * Por Rafael Renó Corrêa, 2025
 * 
 * Aplicação Servidora
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
#include <string.h>     // memset()
#include <signal.h>     // signal()
#include <wait.h>       // waitpid()
#include <stdatomic.h>  // atomic_bool typedef

/*
 *   Definições
 */
#define PORT 8080
#define BUFFER_SIZE 1024

/*
 *   Variáveis Globais
 */
int server_fd,
    client_fd;
volatile atomic_bool running = true;

/*
 *   Assinaturas
 */
void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

void handleSIGTERM(int signal);
// função p/ tratar o sinal de encerramento de processo

int main(int argc, char **argv){
    struct sockaddr_in server_addr;   // endereço do servidor, especialmente para IPv4
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);

    // configura o tratamento de sinais...
    signal(SIGINT,  handleSIGINT);
    signal(SIGCHLD, handleSIGCHLD);

    // cria o soquete do servidor...
    server_fd = socket(AF_INET,     // com protocolo IPv4 e
                       SOCK_STREAM, // baseado em conexão
                       0);
    if(server_fd == -1){
        exit(EXIT_FAILURE);
    }

    // define o endereço do servidor...
    server_addr.sin_family = AF_INET;         // para protocolo IPv4,
    server_addr.sin_addr.s_addr = INADDR_ANY; // de qualquer origem
    server_addr.sin_port = htons(PORT);       // e porta 8080

    // vincula...
    if(bind(server_fd,                      // o file desciptor do soquete do servidor
            server_addr_ptr, // ao endereço do servidor
            server_addr_len) < 0){
        exit(EXIT_FAILURE);
    }

    // declara intenção de escutar novas conexões...
    if(listen(server_fd, // no soquete do servidor com
              5          // fila limite de 5 requisições
             ) < 0){
        exit(EXIT_FAILURE);
    }

    printf("Servidor on-line e ouvindo na porta %d!\n", PORT);

    while(running){
    // loop do servidor

        // aceita uma nova conexão...
        client_fd = accept(server_fd,
                           server_addr_ptr,
                           &server_addr_len);
        if(client_fd < 0){
            sleep(1);

            continue;
        }

        pid_t pid = fork();

        if(pid < 0){
        // fork() falhou
        
            exit(EXIT_FAILURE);
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

                    while(running){
                        ssize_t rcvd = recv(client_fd,
                                            buffer,
                                            BUFFER_SIZE,
                                            0);

                        if(rcvd <= 0){
                            if(rcvd == 0){
                            // conexão perdida
                            
                                break;
                            }else{
                                exit(EXIT_FAILURE);
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

                }
            }

            close(client_fd);
        }else{
        // processo pai

            close(client_fd); // prepara para estabelecer novas conexões
        }
    }

    exit(EXIT_SUCCESS);
}

/*
 *   Funções
 */
void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    close(server_fd);

    running = false;

    exit(EXIT_SUCCESS);
}

void handleSIGCHLD(int signal){
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

    int status;
    pid_t pid;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("\nProcesso filho com PID %d terminou.\n", pid);
    }
}

void handleSIGTERM(int signal){
// função p/ tratar o sinal de encerramento de processo

    printf("\nSinal de término recebido.\n"
           "\nEncerrando processo filho...\n"); // no contexto do processo filho

    exit(EXIT_SUCCESS);
}