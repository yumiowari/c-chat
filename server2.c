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
#include <signal.h>
#include <sys/wait.h>
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
#define LEAF_NODE_MAX 128
/**********/

/* ESTRUTURAS */
struct node{
    int value;
    struct node *next;
};

struct list{
    int qty;
    struct node *root;
};

struct client_info{
    char username[16];
    int client_socket;
    int secret;
};

struct message{
    char buffer[BUFFER_SIZE];
    int secret;
};
/**************/

/* VARIÁVEIS GLOBAIS */
int server_socket; // soquete do servidor
pthread_t tid_in, tid_out; // "thread" id
struct list *leaf_pids; // lista de pids dos processos filhos (nós folha)
/*********************/

/* ASSINATURAS */
struct list* makeList(){
// função p/ alocar o ponteiro p/ a lista
    
    struct list* list = (struct list*) malloc(sizeof(struct list));
    if(list == NULL)return NULL;

    list->root = NULL;
    list->qty = 0;

    return list;
}

struct node* makeNode(int value){
// função p/ alocar o ponteiro para o nó
    
    struct node* new_node = (struct node*) malloc(sizeof(struct node));
    if(new_node == NULL)return NULL;

    new_node->value = value;
    new_node->next = NULL;

    return new_node;
}

bool insertNode(struct list* list, int value){
// função p/ inserir um novo nó na lista

    if(list == NULL)return false;

    struct node* new_node = makeNode(value);
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

bool removeNode(struct list* list, int value){
// função p/ remover um nó da lista

    if(list == NULL)return false;

    struct node *ant, *aux;

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

bool freeList(struct list* list){
// função p/ liberar a lista da memória RAM

    if(list == NULL)return false;

    struct node *ant, *next;

    ant = list->root;

    while(ant != NULL){
        next = ant->next;

        free(ant);

        ant = next;
    }

    free(list);

    return true;
}

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD

void handleSIGTERM(int signal);
// função p/ tratar o sinal de término

bool checkArgs(int argc, char **argv);
// função p/ verificar os parâmetros de entrada

void *handleMsgIn(void *args);
// função p/ lidar com o recebimento de mensagens do cliente

void *handleMsgOut(void *args);
// função p/ lidar com o envio de mensagens ao cliente

struct client_info clientWrapper(int socket, struct message msg);
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
    struct message msg; // estrutura "mensagem" para I/O
    char username[16]; // nome do usuário
    int secret; // segredo
    struct client_info client_id; // identidade do cliente
    pid_t pid; // "process id"

    // inicializa lista de pids dos processos filhos
    leaf_pids = makeList();

    // configura o tratamento do SIGINT
    sa.sa_handler = handleSIGINT; // define a função de tratamento do sinal
    sigemptyset(&sa.sa_mask);     // não bloqueia outros sinais
    sa.sa_flags = 0;              // sem flags adicionais
    if(sigaction(SIGINT, &sa, NULL) == -1){
        fprintf(stderr, RED "ERRO: Falha ao configurar o tratamento do sinal de interrupção.\n" RESET);

        exit(EXIT_FAILURE);
    }

    // configura o tratamento do SIGCHLD
    sa.sa_handler = handleSIGCHLD; // define a função de tratamento do sinal
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP; // Evita interrupção de chamadas bloqueantes
    if(sigaction(SIGCHLD, &sa, NULL) == -1){
        fprintf(stderr, RED "ERRO: Falha ao configurar o tratamento do SIGCHLD.\n" RESET);

        exit(EXIT_FAILURE);
    }

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
        if(recv(client_socket, &msg, sizeof(msg), 0) <= 0){
            fprintf(stderr, RED "ERRO: Falha ao receber a identidade do cliente.\n" RESET);

            exit(EXIT_FAILURE);
        }

        client_id = clientWrapper(client_socket, msg);
        strcpy(username, client_id.username);
        secret = client_id.secret;

        printf(BLUE "%s" RESET " se juntou ao chat!\n", username);
    
        pid = fork();

        if(pid == -1){ // fork() falhou
            fprintf(stderr, RED "ERRO: Criação do processo filho falhou\n" RESET
                                "Conexão com o cliente encerrada.\n");

            close(client_socket);

            continue;
        }else if(pid == 0){ // processo filho
            close(server_socket); // não aceita novas conexões

            // desconfigura o tratamento do SIGINT
            sa.sa_handler = SIG_IGN;
            sigemptyset(&sa.sa_mask);     // não bloqueia outros sinais
            sa.sa_flags = 0;              // sem flags adicionais
            if(sigaction(SIGINT, &sa, NULL) == -1){
                fprintf(stderr, RED "ERRO: Falha ao desconfigurar o tratamento do sinal de interrupção.\n" RESET);
        
                exit(EXIT_FAILURE);
            }

            // configura o tratamento do SIGTERM
            sa.sa_handler = handleSIGTERM;
            sigemptyset(&sa.sa_mask);     // não bloqueia outros sinais
            sa.sa_flags = 0;              // sem flags adicionais
            if(sigaction(SIGTERM, &sa, NULL) == -1){
                fprintf(stderr, RED "ERRO: Falha ao configurar o tratamento do sinal de término.\n" RESET);
        
                exit(EXIT_FAILURE);
            }

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

            // armazena o pid do processo filho na lista
            if(!insertNode(leaf_pids, pid)){
                fprintf(stderr, RED "ERRO: Falha ao inserir o PID na lista.\n");

                exit(EXIT_FAILURE);
            }
        }
    }

    printf(YELLOW "Encerrando servidor...\n" RESET);

    exit(EXIT_SUCCESS);
}

/* FUNÇÕES */
void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção

    struct node *node;

    printf(YELLOW "\nSinal de interrupção recebido.\n"
                  "Encerrando aplicação...\n" RESET);

    close(server_socket);

    // encerra os processos filhos (nós folha)
    if(leaf_pids->qty > 0){
        node = leaf_pids->root;
        while(node != NULL){
            printf("Encerrando processo filho (PID: %d)...\n", node->value);

            kill(node->value, SIGTERM);

            node = node->next;
        }
    }

    freeList(leaf_pids);

    printf(GREEN "Servidor encerrado com sucesso.\n" RESET);

    exit(EXIT_SUCCESS);
}

void handleSIGCHLD(int signal){
// função p/ tratar o SIGCHLD

    int status;
    pid_t pid;

    // `waitpid` com `WNOHANG` evita bloquear caso não haja filhos para limpar
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("Filho com PID %d terminou.\n", pid);

        // remove o PID do processo filho na lista
        if(!removeNode(leaf_pids, pid)){
            fprintf(stderr, RED "ERRO: Falha ao remover o PID da lista.\n");

            exit(EXIT_FAILURE);
        }
    }
}

void handleSIGTERM(int signal){
// função p/ tratar o sinal de término

    printf(YELLOW "\nSinal de término recebido.\n"
                  "Encerrando aplicação...\n" RESET);
    
    // cancela as threads de comunicação
    pthread_cancel(tid_in);
    pthread_cancel(tid_out);
    
    printf(GREEN "Processo filho encerrado com sucesso.\n" RESET);
    
    exit(EXIT_SUCCESS);
}

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

    struct message msg; // estrutura "mensagem" para I/O
    ssize_t recv_bytes; // qtd de bytes recebidos
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    char username[16]; // nome de usuário
    strcpy(username, client_id->username);
    int secret = client_id->secret;

    while(true){
        recv_bytes = recv(client_socket, &msg, sizeof(msg), 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf(YELLOW "Aviso: A conexão com " BLUE "%s" YELLOW " foi perdida.\n" RESET, username);
            }else fprintf(stderr, RED "ERRO: Falha na recepção de dados.\n" RESET);

            break;
        }

        if(msg.secret == secret){
            printf(MAGENTA "%s" RESET ": %s", username, msg.buffer);
        }else{
            fprintf(stderr, RED "ERRO: Código de segurança inválido para o cliente " BLUE "%s.\n" RESET, username);

            break;
        }

        memset(msg.buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);
}

void *handleMsgOut(void *args){
// função p/ lidar com o envio de mensagens ao cliente

    struct message msg; // estrutura "mensagem" para I/O
    struct client_info *client_id = (struct client_info*) args; // identidade do cliente
    int client_socket = client_id->client_socket; // soquete do cliente
    int secret = client_id->secret;
    msg.secret = secret;
    memset(msg.buffer, 0, BUFFER_SIZE);

    while(true){
        if(send(client_socket, &msg, sizeof(msg), 0) == -1){
            fprintf(stderr, RED "ERRO: Falha ao enviar mensagem ao cliente.\n" RESET);

            break;
        }

        sleep(1); // cutuca o cliente a cada segundo
    }

    close(client_socket);

    pthread_exit(NULL);
}

struct client_info clientWrapper(int socket, struct message msg){
// função p/ "embrulhar" as informações do cliente

    struct client_info client_id;

    client_id.client_socket = socket;
    strcpy(client_id.username, msg.buffer);
    client_id.secret = msg.secret;

    return client_id;
}
/***********/