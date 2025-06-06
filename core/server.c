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
#include <stdbool.h>    // bool type
#include <unistd.h>     // fork()
#include <arpa/inet.h>  // inet_pton(), htons(), etc.
#include <sys/socket.h> // socket(), connect(), bind(), listen(), accept()
#include <netinet/in.h> // struct sockaddr_in
#include <omp.h>        // OpenMP
#include <string.h>     // memset()
#include <signal.h>     // signal()
#include <wait.h>       // waitpid()
#include <stdatomic.h>  // atomic_bool typedef
#include <errno.h>      // nº do último erro
#include <sys/ipc.h>    // ftok()
#include <sys/shm.h>    // shared memory
#include <sys/sem.h>    // semaphore

#include "server_utils.h" // server_t
#include "client_utils.h" // client_t, sem_wait(), sem_open()

/*
 *  Definições
 */
#define BUFFER_SIZE 1024
#define SHM_SIZE BUFFER_SIZE
#define MAX_CHILDREN 1024
#define PROJECT_ID 65

/*
 *  Macros
 */
#define FORMAT_ERROR(error, prefix) strcpy(error, prefix); if(errno != 0)strcat(error, strerror(errno));

/*
 *  Variáveis Globais
 */
int server_fd,
    client_fd;
bool running = true;
client_t children[MAX_CHILDREN]; // array de processos filhos (clientes)
int children_qty = 0;                 // contador de filhos (solução temporária)
int secrets[MAX_CHILDREN]; // array de secrets
int secrets_qty = 0;       // contador de secrets

/*
 *  Assinaturas
 */
server_t setupComm(int argc, char **argv);
// módulo p/ estabelecer conexão cliente-servidor

client_t tryAccept(server_t server);
// tentar aceitar uma nova conexão com um cliente

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (CTRL + C)

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

void handleSIGTERM(int signal);
// função p/ tratar o sinal de encerramento de processo

void gracefulShutdown(int context);
// rotina de encerramento gracioso

void crashLanding(int context, char *error);
// rotina de encerramento em caso de falha

void killOffspring();
// função p/ matar os processos filhos

int main(int argc, char **argv){
    char error[BUFFER_SIZE];
    char path[32];
    bool flag;

    // configura o tratamento de sinais...
    signal(SIGINT,  handleSIGINT);
    signal(SIGCHLD, handleSIGCHLD);

    server_t server = setupComm(argc, argv);

    printf("Servidor on-line e ouvindo na porta %d!\n", server.port);

    while(running){
    // loop do servidor

        client_t client = tryAccept(server);

        printf("Conexão estabelecida com %s!\n", client.username);

        // verifica se o grupo do cliente existe
        flag = false;
        for(int i = 0; i < secrets_qty; i++){
            if(secrets[i] == client.secret){
                flag = true;

                break;
            }
        }
        if(flag == false){
        // se não existir, insere no array de grupos

            secrets[secrets_qty] = client.secret;

            secrets_qty++;
        }

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

            // inicia os espaços de memória compartilhados
            sprintf(path, "./tmp/shm_%ld", client.secret);
            FILE *f = fopen(path, "a");
            if(f)fclose(f);

            client.shm_key = ftok(path, PROJECT_ID);

            sprintf(path, "./tmp/sem_%ld", client.secret);
            f = fopen(path, "a");
            if(f)fclose(f);

            client.sem_key = ftok(path, PROJECT_ID);
            
            if(flag == false){
            // é o processo que cria o segmento de memória compartilhado
                
                client.shm_id = shmget(client.shm_key, SHM_SIZE, IPC_CREAT | 0666);
                if(client.shm_id < 0){
                    FORMAT_ERROR(error, "Falha na criação do espaço de memória compartilhado: ");

                    crashLanding(1, error);
                }

                client.sem_id = semget(client.sem_key, 1, IPC_CREAT | 0666);
                if(client.sem_id < 0){
                    FORMAT_ERROR(error, "Falha na criação do semáforo compartilhado: ");

                    crashLanding(1, error);
                }

                union semun sem_attr;
                sem_attr.val = 1;
                if(semctl(client.sem_id, 0, SETVAL, sem_attr) == -1){
                    FORMAT_ERROR(error, "Falha ao atribuir o valor inicial ao semáforo: ");

                    crashLanding(1, error);
                }
            }else{
            // é o processo que acessa o segmento de memória compartilhado

                client.shm_id = shmget(client.shm_key, SHM_SIZE, 0666); // sem IPC_CREAT
                if(client.shm_id < 0){
                    FORMAT_ERROR(error, "Falha no acesso ao espaço de memória compartilhado: ");

                    crashLanding(1, error);
                }

                client.sem_id = semget(client.sem_key, 1, 0666); // sem IPC_CREAT
                if(client.sem_id < 0){
                    FORMAT_ERROR(error, "Falha no acesso ao semáforo compartilhado: ");

                    crashLanding(1, error);
                }
            }
            
            // lógica de comunicação
            #pragma omp parallel sections shared(running) firstprivate(client)
            {
                #pragma omp section
                {
                // entrada

                    while(running){
                    // recebe mensagens do cliente e encaminha para os outros membros do grupo

                        message_t message;
                        memset(message.buffer, 0, 1024);
                        memset(message.username, 0, 16);
                        message.secret = -1;

                        // recebe a mensagem do cliente
                        ssize_t rcvd = recv(client_fd,
                                            &message,
                                            sizeof(message_t),
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

                        #pragma omp critical
                        {
                        printf("(%ld:%d) %s: %s\n", message.secret,
                                                    getpid(),
                                                    message.username,
                                                    message.buffer);
                        }

                        // entra na seção crítica
                        if(sem_wait(client.sem_id) == false){
                            FORMAT_ERROR(error, "Falha ao entrar na seção crítica: ");

                            crashLanding(1, error);
                        }

                        // encaminha a mensagem para os outros membros do grupo
                        client.shm_ptr = (message_t*) shmat(client.shm_id, NULL, 0);
                        if(client.shm_ptr == (message_t*) -1){
                            FORMAT_ERROR(error, "Falha ao anexar o segmento de memória compartilhado: ");

                            crashLanding(1, error);
                        }

                        // escreve na memória compartilhada
                        *client.shm_ptr = message;

                        shmdt(client.shm_ptr); // libera o segmento de memória compartilhado

                        // sai da seção crítica
                        if(sem_open(client.sem_id) == false){
                            FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                            crashLanding(1, error);
                        }
                    }
                }

                #pragma omp section
                {
                // saída

                    message_t old_msg;

                    while(running){
                    // lê mensagens dos outros membros do grupo e encaminha para o cliente

                        message_t message;
                        memset(message.buffer, 0, 1024);
                        memset(message.username, 0, 16);
                        message.secret = -1;

                        // entra na seção crítica
                        if(sem_wait(client.sem_id) == false){
                            FORMAT_ERROR(error, "Falha ao entrar na seção crítica: ");

                            crashLanding(1, error);
                        }

                        // recebe a mensagem de outro membro do grupo
                        client.shm_ptr = (message_t*) shmat(client.shm_id, NULL, 0);
                        if(client.shm_ptr == (message_t*) -1){
                            FORMAT_ERROR(error, "Falha ao acessar o segmento de memória compartilhado: ");

                            crashLanding(1, error);
                        }

                        // lê a memória compartilhada
                        message = *client.shm_ptr;

                        // verifica o buffer da mensagem
                        if(strlen(message.buffer) == 0){

                            shmdt(client.shm_ptr);

                            if(sem_open(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                                crashLanding(1, error);
                            }

                            usleep(100000); // espera 100ms

                            continue; // não há mensagem válida disponível
                        }

                        // compara o buffer lido com o buffer antigo
                        if(strlen(old_msg.buffer) > 0                  &&
                           strcmp(old_msg.buffer, message.buffer) == 0 &&
                           strcmp(old_msg.username, message.username) == 0){

                            shmdt(client.shm_ptr);

                            // sai da seção crítica
                            if(sem_open(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                                crashLanding(1, error);
                            }

                            usleep(100000); // espera 100ms

                            continue; // é uma mensagem repetida
                        }else{
                            old_msg = message;
                        }

                        shmdt(client.shm_ptr);

                        // sai da seção crítica
                        if(sem_open(client.sem_id) == false){
                            FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                            crashLanding(1, error);
                        }

                        // encaminha a mensagem para o cliente
                        ssize_t sent = send(client_fd,
                                            &message,
                                            sizeof(message_t),
                                            0);
                        if(sent < 0){ // em caso de erro, send() retorna -1
                            FORMAT_ERROR(error, "Falha no envio da mensagem ao cliente: ");
                                                    
                            crashLanding(1, error);
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
server_t setupComm(int argc, char **argv){
// módulo p/ estabelecer conexão cliente-servidor

    struct sockaddr_in server_addr;   // endereço do servidor
    struct sockaddr *server_addr_ptr  // ponteiro genérico para o endereço do servidor
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    char error[BUFFER_SIZE];
    server_t server;

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

client_t tryAccept(server_t server){
// tentar aceitar uma nova conexão com um cliente

    char error[BUFFER_SIZE];

    while(running){
        client_fd = accept(server_fd,
                           server.server_addr_ptr,
                           &server.server_addr_len);
        if(client_fd < 0){
            sleep(1); // espera 1s

            continue;
        }

        // recebe os dados do cliente
        client_t client;
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
    bool flag;
    client_t client;

    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        printf("\nProcesso filho com PID %d terminou.\n", pid);

        for(int i = 0; i < children_qty; i++){ // O(n * m)
            if(children[i].pid == pid){
                client = children[i];

                // remove o processo filho do array
                for(int j = i; j < children_qty - 1; j++)
                    children[j] = children[j + 1];

                children_qty--;

                flag = false;
                for(int j = 0; j < children_qty; j++){
                    if(children[j].secret == client.secret){
                        flag = true;
                    }
                }
                if(flag == false){
                // se não houver outros membros no grupo

                    // remove o segredo do array
                    for(int j = i; j < secrets_qty - 1; j++)
                        secrets[j] = secrets[j + 1];

                    secrets_qty--;
                }

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
            while(children_qty > 0); // espera encerrar todos os processos filhos
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

            gracefulShutdown(context);

            break;

        case 1:
        // processo filho

            fprintf(stderr, "%s\n", error);
            fprintf(stderr, "Fim abrupto do processo %d.\n", getpid());

            gracefulShutdown(context);

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