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

#include <arpa/inet.h> // manipulação de endereços de rede

#include <unistd.h>    // manipulação de processos
#include <pthread.h>   // manipulação de threads
#include <signal.h>    // manipulação de sinais
#include <sys/wait.h>  // controle de processos filhos

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

#define SERVER_LIMIT 16    // limite máximo de usuários

/**********/



/* ESTRUTURAS */

struct node{
    int value;
    struct node *next;
}typedef(Node); // nó

struct list{
    int qty;
    Node *root;
}typedef(List); // lista

struct client_info{
    char username[16];
    int client_socket;
    int secret;
}typedef(Client); // identidade do cliente

struct message{
    char buffer[BUFFER_SIZE];
    int secret;
}typedef(Message); // mensagem

/**************/



/* VARIÁVEIS GLOBAIS */

int server_socket;         // soquete de servidor
pthread_t tid_in, tid_out; // "thread" id
List *leaves;              // lista de PIDs dos processos filhos (i. e., nós folha)

/*********************/



/* ASSINATURAS */

List *makeList(){
// função p/ alocar o ponteiro para a lista
    
    List *list = (List*) malloc(sizeof(List));
    if(list == NULL)return NULL;

    list->root = NULL;
    list->qty = 0;

    return list;
}

Node *makeNode(int value){
// função p/ alocar o ponteiro para o nó
    
    Node *new_node = (Node*) malloc(sizeof(Node));
    if(new_node == NULL)return NULL;

    new_node->value = value;
    new_node->next = NULL;

    return new_node;
}

bool insertNode(List *list, int value){
// função p/ inserir um novo nó na lista

    if(list == NULL)return false;

    Node *new_node = makeNode(value);
    if(new_node == NULL)return false;

    if(list->qty > 0){
        new_node->next = list->root;

        list->root = new_node;
    }else{ // se é o 1º nó
        list->root = new_node;
    }

    list->qty++;

    return true;
}

bool removeNode(List *list, int value){
// função p/ remover um nó da lista

    if(list == NULL)return false;

    Node *ant, *aux;

    if(list->qty > 0){
        aux = list->root;

        if(aux->value != value){
            while((aux != NULL) && (aux->value != value)){
                ant = aux;

                aux = aux->next;
            }

            if(aux != NULL){
                if(aux->next != NULL){
                    aux->next = aux->next;
                }else{ // é o último elemento
                    ant->next = NULL;
                }

                free(aux);
            }else return false; // o nó não está na lista
        }else{ // se é o 1º nó
            list->root = aux->next;

            free(aux);
        }
    }else return false; // lista vazia

    list->qty--;

    return true;
}

bool freeList(List *list){
// função p/ liberar a lista da memória RAM

    if(list == NULL)return false;

    Node *ant, *next;

    ant = list->root;

    while(ant != NULL){
        next = ant->next;

        free(ant);

        ant = next;
    }

    free(list);

    return true;
}

bool checkArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

void handleSIGTERM(int signal);
// função p/ tratar o sinal de encerramento de processo

void *handleMsgIn(void *args);
// função p/ lidar com o recebimento de mensagens do cliente

void *handleMsgOut(void *args);
// função p/ lidar com o envio de mensagens ao cliente

Client clientWrapper(int socket, Message msg);
// função p/ "embrulhar" as informações do cliente

/***************/



int main(int argc, char **argv){
// uso: ./server <porta>

    struct sigaction sa; // signal action
    unsigned short int port; // porta (0 - 65535)
    struct sockaddr_in server_addr; // endereço do servidor
    int client_socket; // soquete do cliente
    struct sockaddr_in client_addr; // endereço do cliente
    socklen_t client_addr_len = sizeof(client_addr); // tamanho do endereço do cliente
    Message msg; // estrutura "mensagem" para I/O
    char username[16]; // nome do usuário
    int secret; // segredo
    Client client_id; // identidade do cliente
    pid_t pid; // "process id"

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){
        port = atoi(argv[1]);
    }else exit(EXIT_FAILURE);

    leaves = makeList(); // inicializa lista de pids dos processos filhos

    // configura o tratamento do SIGINT
    sa.sa_handler = handleSIGINT; // define a função de tratamento do sinal
    sigemptyset(&sa.sa_mask);     // não bloqueia outros sinais
    sa.sa_flags = 0;              // sem flags adicionais
    if(sigaction(SIGINT, &sa, NULL) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na configuração do tratamento do SIGINT.\n");

        exit(EXIT_FAILURE);
    }

    // configura o tratamento do SIGCHLD
    sa.sa_handler = handleSIGCHLD;           // define a função de tratamento do sinal
    sigemptyset(&sa.sa_mask);                // não bloqueia outros sinais
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // evita interrupção de chamadas bloqueantes
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na configuração do tratamento do SIGCHLD.\n");

        exit(EXIT_FAILURE);
    }

    printf("Criando soquete de servidor...\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(server_socket == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na criação do soquete de servidor.\n");

        exit(EXIT_FAILURE);
    }

    printf("Configurando endereço de servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // aceita requisições de qualquer IP
    server_addr.sin_port = htons(port);

    printf("Vinculando o soquete de servidor à porta " GREEN "%d" RESET "...\n", port);
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na vinculação do soquete de servidor à porta " RED "%d" RESET ".\n", port);

        exit(EXIT_FAILURE);
    }

    printf("Iniciando o servidor...\n");
    if(listen(server_socket, 5) == -1){
        fprintf(stderr, RED "ERRO: " RESET "Falha na inicialização do servidor.\n");

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nServidor on-line e ouvindo na porta %d!\n" RESET, port);

    // loop do servidor
    while(true){
        // tenta aceitar uma nova conexão
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if(client_socket == -1){
            fprintf(stderr, RED "ERRO: " RESET "Falha no estabelecimento de conexão com o cliente.\n");

            continue; // tenta novamente
        }

        // configura a identidade do cliente
        if(recv(client_socket, &msg, sizeof(msg), 0) <= 0){
            fprintf(stderr, RED "ERRO: " RESET "Falha na recepção da identidade do cliente.\n");

            exit(EXIT_FAILURE);
        }

        client_id = clientWrapper(client_socket, msg);
        strcpy(username, client_id.username);
        secret = client_id.secret;

        printf(BLUE "%s" RESET " juntou-se ao chat!\n", username);
    
        pid = fork();

        if(pid == -1){ // fork() falhou
            fprintf(stderr, RED "ERRO: " RESET "Criação do processo filho falhou\n"
                                "A conexão com o cliente foi encerrada.\n");

            close(client_socket);

            continue; // tenta estabelecer uma nova conexão
        }else if(pid == 0){ // fork() sucedeu
        // contexto do processo filho

            close(server_socket); // não aceita novas conexões

            // configura o tratamento do SIGINT
            sa.sa_handler = SIG_IGN;  // ignora o sinal (não atribui função de tratamento)
            sigemptyset(&sa.sa_mask); // não bloqueia outros sinais
            sa.sa_flags = 0;          // sem flags adicionais
            if(sigaction(SIGINT, &sa, NULL) == -1){
                fprintf(stderr, RED "ERRO: " RESET "Falha na configuração do tratamento do SIGINT.\n");
        
                exit(EXIT_FAILURE);
            }

            // configura o tratamento do SIGTERM
            sa.sa_handler = handleSIGTERM; // define a função de tratamento do sinal
            sigemptyset(&sa.sa_mask);      // não bloqueia outros sinais
            sa.sa_flags = 0;               // sem flags adicionais
            if(sigaction(SIGTERM, &sa, NULL) == -1){
                fprintf(stderr, RED "ERRO: " RESET "Falha na configuração do tratamento do SIGTERM.\n");
        
                exit(EXIT_FAILURE);
            }

            /* lógica de comunicação com o cliente */
            if(pthread_create(&tid_in, NULL, handleMsgIn, &client_id) != 0){
                fprintf(stderr, RED "ERRO: " RESET "Falha na criação da thread para escutar o cliente.\n");

                exit(EXIT_FAILURE);
            }

            if(pthread_create(&tid_out, NULL, handleMsgOut, &client_id) != 0){
                fprintf(stderr, RED "ERRO: " RESET "Falha na criação da thread para falar ao cliente.\n");

                exit(EXIT_FAILURE);
            }
                
            if(pthread_join(tid_in, NULL) != 0){
            // espera o fim da conexão com o cliente

                fprintf(stderr, RED "ERRO: " RESET "Falha no aguardo da thread de recebimento de mensagens.\n");

                pthread_cancel(tid_out); // tenta encerrar a thread de envio se a thread de recebimento falhou

                exit(EXIT_FAILURE);
            }

            printf(BLUE "%s" RESET " saiu do chat!\n", username);

            printf(YELLOW "Encerrando processo filho...\n" RESET);

            // cancela as threads de comunicação
            pthread_cancel(tid_in);
            pthread_cancel(tid_out);

            exit(EXIT_SUCCESS);
        }else{ // fork() sucedeu
        // contexto do processo pai

            close(client_socket); // prepara para estabelecer uma nova conexão

            // armazena o PID do processo filho na lista
            if(!insertNode(leaves, pid)){
                fprintf(stderr, RED "ERRO: " RESET "Falha na inserção do PID do processo filho na lista.\n");

                exit(EXIT_FAILURE);
            }
        }
    }
}



/* FUNÇÕES */

bool checkArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada
    
    if(argc < 2){
        fprintf(stderr, RED "ERRO: " RESET "Argumentos insuficientes.\n"
                            "Uso: ./server <porta>\n");
    
        return false;
    }else if(argc > 2){
        printf(YELLOW "Aviso: " RESET "Argumentos excedentes.\n"
                      "Uso: ./server <porta>\n");
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
    
    return true;
}

void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (CTRL + C)

    Node *node;

    printf(YELLOW "\nAviso: " RESET "Sinal de interrupção recebido.\n"
                  "Encerrando aplicação...\n");

    close(server_socket);

    // encerra os processos filhos (nós folha)
    if(leaves->qty > 0){
        node = leaves->root;
        while(node != NULL){
            printf(YELLOW "Aviso: " RESET "Encerrando processo filho (PID: " YELLOW "%d" RESET ")...\n", node->value);

            kill(node->value, SIGTERM);

            node = node->next;
        }
    }

    freeList(leaves);

    printf(GREEN "Servidor encerrado com sucesso.\n" RESET);

    exit(EXIT_SUCCESS);
}

void handleSIGCHLD(int signal){
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

    int status;
    pid_t pid;

    // "waitpid" com "WNOHANG" evita bloqueio caso não haja filhos para limpar
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf(YELLOW "\nAviso: " RESET "Filho com PID " YELLOW "%d" RESET " terminou.\n", pid);

        // remove o PID do processo filho da lista
        if(!removeNode(leaves, pid)){
            fprintf(stderr, RED "ERRO: " RESET "Falha na remoção do PID do processo filho da lista.\n");

            exit(EXIT_FAILURE);
        }
    }
}

void handleSIGTERM(int signal){
// função p/ tratar o sinal de encerramento de processo

    printf(YELLOW "\nAviso: " RESET "Sinal de término recebido.\n"
                  "Encerrando aplicação...\n");
    
    // cancela as threads de comunicação
    pthread_cancel(tid_in);
    pthread_cancel(tid_out);
    
    printf(GREEN "Processo filho encerrado com sucesso.\n" RESET);
    
    exit(EXIT_SUCCESS);
}

void *handleMsgIn(void *args){
// função p/ lidar com o recebimento de mensagens do cliente

    Message msg; // estrutura "mensagem" para I/O
    ssize_t recv_bytes; // qtd de bytes recebidos
    Client *client_id = (Client*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    char username[16]; // nome de usuário
    strcpy(username, client_id->username);
    int secret = client_id->secret;

    while(true){
        recv_bytes = recv(client_socket, &msg, sizeof(msg), 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf(YELLOW "Aviso: " RESET "A conexão com " BLUE "%s" RESET " foi perdida.\n", username);
            }else fprintf(stderr, RED "ERRO: " RESET "Falha na recepção de dados.\n");

            break;
        }

        if(msg.secret == secret){
            printf(MAGENTA "%s" RESET ": %s", username, msg.buffer);
        }else{
            fprintf(stderr, RED "ERRO: " RESET "Código de segurança inválido para o cliente " BLUE "%s" RESET ".\n", username);

            break;
        }

        memset(msg.buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

void *handleMsgOut(void *args){
// função p/ lidar com o envio de mensagens ao cliente

    Message msg; // estrutura "mensagem" para I/O
    Client *client_id = (Client*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    int secret = client_id->secret;
    msg.secret = secret;
    memset(msg.buffer, 0, BUFFER_SIZE);

    while(true){
        if(send(client_socket, &msg, sizeof(msg), 0) == -1){
            fprintf(stderr, RED "ERRO: " RESET "Falha no envio de mensagem ao cliente.\n");

            break;
        }

        sleep(1); // cutuca o cliente a cada segundo
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