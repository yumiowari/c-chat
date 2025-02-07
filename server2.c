//////////////////////////////////////////////////
//                                              //
//     Sistema de Chatting Cliente-Servidor     //
//                                              //
//         Por Rafael Renó Corrêa, 2025         //
//                                              //
//////////////////////////////////////////////////

/* BIBLIOTECAS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
/***************/

/* MACROS */
#define RESET   "\x1B[0m"

#define BLACK   "\x1B[30m"
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define WHITE   "\x1B[37m"

#define BUFFER_SIZE 1024
/**********/

/* ESTRUTURAS */
struct client_info{
    char username[16];
    int client_socket;
    int secret;
};
/**************/

/* ASSINATURAS */
bool checkArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

void *handleMsgIn(void *args);
// função p/ lidar com o recebimento de mensagens do cliente

void *handleMsgOut(void *args);
// função p/ lidar com o envio de mensagens ao cliente
/***************/

int main(int argc, char **argv){
// uso: ./server <porta>

    unsigned short int port; // porta (0 - 65535)
    int server_socket; // soquete do servidor
    struct sockaddr_in server_addr; // endereço do servidor
    int client_socket; // soquete do cliente
    struct sockaddr_in client_addr; // endereço do cliente
    socklen_t client_addr_len = sizeof(client_addr); // tamanho do endereço do cliente
    char buffer[BUFFER_SIZE]; // buffer para I/O
    char *token;
    int i; // contador
    char username[16]; // nome do usuário
    int secret;
    struct client_info client_id; // identidade do cliente
    pid_t pid; // "process id"
    pthread_t tid_in, tid_out; // "thread id"

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){
        port = atoi(argv[1]);
    }else exit(EXIT_FAILURE);

    printf("Criando soquete do servidor...\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(server_socket == -1){
        fprintf(stderr, RED "ERRO: Falha na criação do soquete do servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    printf("Configurando endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // aceita requisições de qualquer IP
    server_addr.sin_port = htons(port);

    printf("Vinculando o soquete do servidor à porta %d...\n", port);
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: Falha ao vincular o soquete do servidor à porta %d.\n" RESET, port);

        exit(EXIT_FAILURE);
    }

    printf("Iniciando o servidor...\n");
    if(listen(server_socket, 5) == -1){
        fprintf(stderr, RED "ERRO: Falha ao iniciar o servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nServidor on-line e ouvindo na porta %d!\n" RESET, port);

    // loop do servidor
    while(true){
        // tenta aceitar uma nova conexão
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_socket == -1){
            fprintf(stderr, RED "ERRO: Falha ao aceitar conexão com um cliente.\n" RESET);

            continue; // tenta novamente
        }

        // configura a identidade do cliente
        if(recv(client_socket, buffer, BUFFER_SIZE, 0) <= 0){
            fprintf(stderr, RED "ERRO: Falha ao receber a identidade do cliente.\n" RESET);

            exit(EXIT_FAILURE);
        }

        i = 0;
        token = strtok(buffer, " ");
        while(token != NULL){
            if(i == 0){ // 1ª iteração
                strcpy(username, token);
                username[strlen(token)] = '\0';
            }

            if(i == 1){ // 2ª iteração
                secret = atoi(token);
            }

            i++;

            token = strtok(NULL, " ");
        }

        client_id.client_socket = client_socket;
        strcpy(client_id.username, username);
        client_id.secret = secret;

        printf(BLUE "%s" RESET " se juntou ao chat!\n", username);
    
        pid = fork();

        if(pid == -1){ // fork() falhou
            fprintf(stderr, RED "ERRO: Criação do processo filho falhou\n" RESET
                                "Conexão com o cliente encerrada.\n");

            close(client_socket);

            continue;
        }else if(pid == 0){ // processo filho
            close(server_socket); // não aceita novas conexões

            /* lógica de comunicação com o cliente */
            if(pthread_create(&tid_in, NULL, handleMsgIn, &client_id) != 0){
                fprintf(stderr, RED "ERRO: Falha ao criar thread para escutar o cliente.\n" RESET);

                exit(EXIT_FAILURE);
            }

            if(pthread_create(&tid_out, NULL, handleMsgOut, &client_id) != 0){
                fprintf(stderr, RED "ERRO: Falha ao criar thread para falar ao cliente.\n" RESET);

                exit(EXIT_FAILURE);
            }
                
            if(pthread_join(tid_in, NULL) != 0){
            // espera o fim da conexão com o cliente
                fprintf(stderr, RED "ERRO: Falha ao aguardar a thread de recebimento de mensagens.\n" RESET);

                pthread_cancel(tid_out); // tenta encerrar a thread de envio se a thread de recebimento falhou

                exit(EXIT_FAILURE);
            }

            printf(BLUE "%s" RESET " saiu do chat!\n", username);

            printf("Encerrando processo filho...\n");

            pthread_cancel(tid_in);
            pthread_cancel(tid_out);

            exit(EXIT_SUCCESS);
        }else{ // processo pai
            close(client_socket); // prepara para estabelecer uma nova conexão
        }
    }

    printf(YELLOW "Encerrando servidor...\n" RESET);

    exit(EXIT_SUCCESS);
}

/* FUNÇÕES */
bool checkArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada

    if(argc < 2){
        fprintf(stderr, RED "ERRO: Argumentos insuficientes.\n" RESET
                            "Uso: ./server <porta>\n");

        return false;
    }else if(argc > 2){
        printf(YELLOW "Aviso: Argumentos excedentes.\n" RESET
                      "Uso: ./server <porta>\n");
    }
        
    for(int i = 0; i < strlen(argv[1]); i++){
        if(!isdigit(argv[1][i])){
            fprintf(stderr, RED "ERRO: A porta deve ser um inteiro.\n" RESET);

            return false;
        }
    }

    if(atoi(argv[1]) > 65535 || atoi(argv[1]) < 0){
        fprintf(stderr, RED "ERRO: A porta deve ser um inteiro positivo entre 0 e 65535.\n" RESET);
        
        return false;
    }

    return true;
}

void *handleMsgIn(void *args){
// função p/ lidar com o recebimento de mensagens do cliente

    char buffer[BUFFER_SIZE]; // buffer para I/O
    ssize_t recv_bytes; // qtd de bytes recebidos
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    char username[16]; // nome de usuário
    strcpy(username, client_id->username);

    while(true){
        recv_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf(YELLOW "Aviso: A conexão com " BLUE "%s" YELLOW " foi perdida.\n" RESET, username);
            }else fprintf(stderr, RED "ERRO: Falha na recepção de dados.\n" RESET);

            break;
        }

        buffer[recv_bytes] = '\0';

        printf(MAGENTA "%s" RESET ": %s", username, buffer);

        memset(buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

void *handleMsgOut(void *args){
// função p/ lidar com o envio de mensagens ao cliente

    char buffer[BUFFER_SIZE]; // buffer para I/O
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    int secret = client_id->secret;

    sprintf(buffer, "%d", secret);

    while(true){
        if(send(client_socket, buffer, strlen(buffer) + 1, 0) == -1){
            fprintf(stderr, RED "ERRO: Falha ao enviar mensagem ao cliente.\n" RESET);

            break;
        }

        sleep(1); // cutuca o cliente a cada segundo
    }

    close(client_socket);

    pthread_exit(NULL);
}
/***********/