//////////////////////////////////////////////////
//                                              //
//     Sistema de Chatting Cliente-Servidor     //
//                                              //
//         Por Rafael Renó Corrêa, 2025         //
//                                              //
//////////////////////////////////////////////////



/* BIBLIOTECAS */

#include <stdio.h>     // funcionalidades de I/O
#include <stdlib.h>    // manipulação de memória
#include <string.h>    // manipulação de strings
#include <ctype.h>     // manipulação de caracteres
#include <stdbool.h>   // definição de tipo booleano
#include <time.h>      // manipulação de data e hora
#include <limits.h>    // definição de limites e constantes

#include <arpa/inet.h> // manipulação de endereços de rede

#include <unistd.h>    // manipulação de processos
#include <pthread.h>   // manipulação de threads
#include <signal.h>    // manipulação de sinais

/***************/



/* MACROS */

#define RESET   "\x1B[0m"  // cor padrão

#define BLACK   "\x1B[30m" // cor preta
#define RED     "\x1B[31m" // cor vermelha
#define GREEN   "\x1B[32m" // cor verde
#define YELLOW  "\x1B[33m" // cor amarela
#define BLUE    "\x1B[34m" // cor azul
#define MAGENTA "\x1B[35m" // cor magenta
#define CYAN    "\x1B[36m" // cor ciano
#define WHITE   "\x1B[37m" // cor branca

#define BUFFER_SIZE 1024   // tamanho do buffer de I/O

#define SERVER_IP "127.0.0.1" // endereço IPv4 do servidor

/*********/



/* ESTRUTURAS */

struct client_info{
    char username[16];
    int client_socket;
    int secret;
}typedef(Client); // identidade de cliente

struct message{
    char buffer[BUFFER_SIZE];
    int secret;
}typedef(Message); // mensagem

/**************/



/* VARIÁVEIS GLOBAIS */

int client_socket;         // soquete de cliente
pthread_t tid_in, tid_out; // "thread" id

/*********************/



/* ASSINATURAS */

bool checkArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void *handleMsgIn(void *args);
// função p/ lidar com o recebimento de mensagens do servidor

void *handleMsgOut(void *args);
// função p/ lidar com o envio de mensagens ao servidor

unsigned int random_int(){
// função p/ gerar um número inteiro aleatório

    return rand();
}

Client clientWrapper(int socket, Message msg);
// função p/ "embrulhar" as informações do cliente

/***************/



int main(int argc, char **argv){
// uso: ./client <porta> <nome de usuário>

    struct sigaction sa; // signal action
    unsigned short int port; // porta (0 - 65535)
    int secret; // segredo
    char username[16]; // nome de usuário
    Message msg; // estrutura "mensagem" para I/O
    struct sockaddr_in server_addr; // endereço do servidor
    Client client_id; // identidade de cliente

    srand(time(NULL)); // inicializa a semente p/ rand()
    secret = random_int();

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){
        port = atoi(argv[1]);
        strcpy(username, argv[2]);
    }else exit(EXIT_FAILURE);

    // configura o tratamento do SIGINT
    sa.sa_handler = handleSIGINT; // define a função de tratamento do sinal
    sigemptyset(&sa.sa_mask);     // não bloqueia outros sinais
    sa.sa_flags = 0;              // sem flags adicionais
    if(sigaction(SIGINT, &sa, NULL) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na configuração do tratamento do SIGINT.\n");

        exit(EXIT_FAILURE);
    }
    
    printf("Criando soquete de cliente...\n");
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(client_socket == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na criação do soquete de cliente.\n");

        exit(EXIT_FAILURE);
    }

    printf("Configurando endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);

    printf("Estabelecendo conexão com o servidor...\n");
    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha no estabelecimento de conexão com o servidor.\n");

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nConexão estabelecida com o servidor!\n" RESET);

    // informa o identidade de cliente ao servidor
    strcpy(msg.buffer, username);
    msg.secret = secret;
    if(send(client_socket, &msg, sizeof(msg), 0) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha no envio da identidade de cliente ao servidor.\n");

        exit(EXIT_FAILURE);
    }

    client_id = clientWrapper(client_socket, msg);

    memset(msg.buffer, 0, BUFFER_SIZE); // limpa o buffer

    /* lógica de comunicação com o servidor */
    if(pthread_create(&tid_in, NULL, handleMsgIn, &client_id) != 0){
        fprintf(stderr, RED "ERRO: " RESET "Falha na criação da thread para escutar o servidor.\n");

        exit(EXIT_FAILURE);
    }

    if(pthread_create(&tid_out, NULL, handleMsgOut, &client_id) != 0){
        fprintf(stderr, RED "ERRO: " RESET "Falha na criação da thread para falar ao servidor.\n");

        exit(EXIT_FAILURE);
    }

    if(pthread_join(tid_in, NULL) != 0){
    // espera o fim da conexão com o servidor

        fprintf(stderr, RED "ERRO: " RESET "Falha no aguardo da thread de recebimento de mensagens.\n");

        pthread_cancel(tid_out); // tenta encerrar a thread de envio se a thread de recebimento falhou

        exit(EXIT_FAILURE);
    }
    //

    printf(YELLOW "Encerrando aplicação...\n" RESET);

    pthread_cancel(tid_in);
    pthread_cancel(tid_out);

    exit(EXIT_SUCCESS);
}



/* FUNÇÕES */

bool checkArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada

    if(argc < 3){
        fprintf(stderr, RED "ERRO: " RESET "Argumentos insuficientes.\n"
                            "Uso: ./client <porta> <nome de usuário>\n");

        return false;
    }else if(argc > 3){
        printf(YELLOW "Aviso: " RESET "Argumentos excedentes.\n"
                      "Uso: ./client <porta> <nome de usuário>\n");
    }
        
    for(int i = 0; i < strlen(argv[1]); i++){
        if(!isdigit(argv[1][i])){
            fprintf(stderr, RED "ERRO: " RESET "A porta deve ser um número inteiro.\n");

            return false;
        }
    }

    if(atoi(argv[1]) > 65535 || atoi(argv[1]) < 0){
        fprintf(stderr, RED "ERRO: " RESET "A porta deve ser um número inteiro entre 0 e 65535.\n");
        
        return false;
    }

    if(strlen(argv[2]) > 15){
        fprintf(stderr, RED "ERRO: " RESET "O nome de usuário não pode exceder 15 caracteres.\n");

        return false;
    }

    for(int i = 0; i < strlen(argv[2]); i++){
        if(
           ((argv[2][i] < 'A') || (argv[2][i] > 'Z')) &&
           ((argv[2][i] < 'a') || (argv[2][i] > 'z')) &&
           (argv[2][i] != '_')
          ){
            fprintf(stderr, RED "ERRO: " RESET "O nome de usuário contém símbolos proibidos.\n"
                                "São caracteres válidos: A-Z a-z _\n");

            return false;
        }
    }

    return true;
}

void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)
    
    printf(YELLOW "\nAviso: " RESET "Sinal de interrupção recebido.\n"
                  "Encerrando aplicação...\n");
    
    // cancela as threads de comunicação
    pthread_cancel(tid_in);
    pthread_cancel(tid_out);

    close(client_socket);
    
    printf(GREEN "Aplicação encerrada com sucesso.\n" RESET);
    
    exit(EXIT_SUCCESS);
}

void *handleMsgIn(void *args){
// função p/ lidar com o recebimento de mensagens do servidor

    Message msg; // estrutura "mensagem" para I/O
    ssize_t recv_bytes; // qtd de bytes recebidos
    Client *client_id = (Client*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    int secret = client_id->secret; // segredo
    int recv_secret; // segredo "recebido"

    while(true){
        recv_bytes = recv(client_socket, &msg, sizeof(msg), 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf(YELLOW "\nAviso: " RESET "A conexão com servidor foi perdida.\n");
            }else fprintf(stderr, RED "\nERRO: " RESET "Falha na recepção de dados.\n");

            break;
        }

        recv_secret = msg.secret;

        if(recv_secret != secret){
            printf(YELLOW "\nAviso: " RESET "O servidor terminou a conexão.\n");

            break;
        }

        memset(msg.buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

void *handleMsgOut(void *args){
// função p/ lidar com o envio de mensagens ao servidor

    char buffer[BUFFER_SIZE]; // buffer para I/O
    Client *client_id = (Client*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    Message msg; // estrutura "mensagem" para I/O
    msg.secret = client_id->secret; // segredo

    while(true){
        printf(MAGENTA "> " RESET);
        fgets(msg.buffer, BUFFER_SIZE, stdin);

        if(send(client_socket, &msg, sizeof(msg), 0) == -1){
            fprintf(stderr, RED "ERRO: " RESET "Falha no envio da mensagem ao servidor.\n");

            break;
        }

        memset(msg.buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

Client clientWrapper(int socket, Message msg){
// função p/ "embrulhar" as informações do cliente

    Client client_id;

    client_id.client_socket = socket;
    strcpy(client_id.username, msg.buffer);
    client_id.secret = msg.secret;

    return client_id;
}

/***********/