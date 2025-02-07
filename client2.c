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
#include <time.h>
#include <limits.h>
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

#define SERVER_IP "127.0.0.1"

#define BUFFER_SIZE 1024
/*********/

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
// função p/ lidar com o recebimento de mensagens do servidor

void *handleMsgOut(void *args);
// função p/ lidar com o envio de mensagens ao servidor

unsigned int random_int(){
    return rand();
}
// função p/ gerar um número inteiro aleatório (0 - RAND_MAX)
/***************/

int main(int argc, char **argv){
// uso: ./client <porta> <nome de usuário>

    unsigned short int port; // porta (0 - 65535)
    char username[16]; // nome de usuário
    int client_socket; // soquete de cliente
    struct sockaddr_in server_addr; // endereço do servidor
    char buffer[BUFFER_SIZE]; // buffer para I/O
    pthread_t tid_in, tid_out; // "thread" id
    struct client_info client_id;
    int secret;

    srand(time(NULL)); // inicializa a semente p/ rand()
    secret = random_int();
    client_id.secret = secret;

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){
        port = atoi(argv[1]);
        strcpy(username, argv[2]);
    }else exit(EXIT_FAILURE);
    strcpy(client_id.username, username);

    printf("Criando soquete de cliente...\n");
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(client_socket == -1){
        fprintf(stderr, RED "ERRO: Falha na criação do soquete do servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }
    client_id.client_socket = client_socket;

    printf("Configurando endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);

    printf("Estabelecendo conexão com o servidor...\n");
    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: Falha ao estabelecer conexão com o servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nConexão estabelecida com o servidor!\n" RESET);

    // informa o nome de usuário e o segredo ao servidor
    sprintf(buffer, "%s %d", username, secret);
    if(send(client_socket, buffer, BUFFER_SIZE, 0) == -1){
        fprintf(stderr, RED "ERRO: Falha ao informar o nome de usuário e o segredo ao servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, BUFFER_SIZE); // limpa o buffer

    /* lógica de comunicação com o servidor */
    if(pthread_create(&tid_in, NULL, handleMsgIn, &client_id) != 0){
        fprintf(stderr, RED "ERRO: Falha ao criar thread para escutar o servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    if(pthread_create(&tid_out, NULL, handleMsgOut, &client_id) != 0){
        fprintf(stderr, RED "ERRO: Falha ao criar thread para falar ao servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    if(pthread_join(tid_in, NULL) != 0){
    // espera o fim da conexão com o servidor
        fprintf(stderr, RED "ERRO: Falha ao aguardar a thread de recebimento de mensagens.\n" RESET);

        pthread_cancel(tid_out); // tenta encerrar a thread de envio se a thread de recebimento falhou

        exit(EXIT_FAILURE);
    }
    //

    printf(YELLOW "Encerrando cliente...\n" RESET);

    pthread_cancel(tid_in);
    pthread_cancel(tid_out);

    exit(EXIT_SUCCESS);
}

/* FUNÇÕES */
bool checkArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada

    if(argc < 3){
        fprintf(stderr, RED "ERRO: Argumentos insuficientes.\n" RESET
                            "Uso: ./client <porta> <nome de usuário>\n");

        return false;
    }else if(argc > 3){
        printf(YELLOW "Aviso: Argumentos excedentes.\n" RESET
                      "Uso: ./client <porta> <nome de usuário>\n");
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

    if(strlen(argv[2]) > 15){
        fprintf(stderr, RED "ERRO: O nome de usuário não pode exceder 15 caracteres.\n" RESET);

        return false;
    }

    for(int i = 0; i < strlen(argv[2]); i++){
        if(
           ((argv[2][i] < 'A') || (argv[2][i] > 'Z')) &&
           ((argv[2][i] < 'a') || (argv[2][i] > 'z')) &&
           (argv[2][i] != '_')
          ){
            fprintf(stderr, RED "ERRO: O nome de usuário contém símbolos proibidos.\n" RESET
                                "São caracteres válidos: A-Z a-z _\n");

            return false;
        }
    }

    return true;
}

void *handleMsgIn(void *args){
// função p/ lidar com o recebimento de mensagens do servidor

    char buffer[BUFFER_SIZE]; // buffer para I/O
    ssize_t recv_bytes; // qtd de bytes recebidos
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    int secret = client_id->secret;
    int recv_secret;

    while(true){
        recv_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf(YELLOW "\nAviso: A conexão com servidor foi perdida.\n" RESET);
            }else fprintf(stderr, RED "\nERRO: Falha na recepção de dados.\n" RESET);

            break;
        }

        buffer[recv_bytes] = '\0';

        recv_secret = atoi(buffer);

        if(recv_secret != secret){
            printf(YELLOW "\nAviso: O servidor terminou a conexão.\n" RESET);

            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

void *handleMsgOut(void *args){
// função p/ lidar com o envio de mensagens ao servidor

    char buffer[BUFFER_SIZE]; // buffer para I/O
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente

    while(true){
        printf(MAGENTA "> " RESET);
        fgets(buffer, BUFFER_SIZE, stdin);

        if(send(client_socket, buffer, strlen(buffer) + 1, 0) == -1){
            fprintf(stderr, RED "ERRO: Falha ao enviar mensagem ao servidor.\n" RESET);

            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}
/***********/