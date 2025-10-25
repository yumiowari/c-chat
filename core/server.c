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
#include <errno.h>      // nº do último erro
#include <sys/ipc.h>    // ftok()
#include <sys/shm.h>    // shared memory
#include <sys/sem.h>    // semaphore

#include "server_utils.h" // server_t, sem_wait(), sem_open()
#include "client_utils.h" // client_t, message_t

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
// descritores de arquivo dos soquetes
int server_fd,
    client_fd;
bool running = true;
client_t children[MAX_CHILDREN]; // array de processos filhos (clientes)
int children_qty = 0;            // contador de filhos (solução temporária)
int secrets[MAX_CHILDREN]; // array de secrets
int secrets_qty = 0;       // contador de secrets

/*
 *  Assinaturas
 */
server_t setupComm(int argc, char **argv);
// módulo p/ estabelecer conexão cliente-servidor

client_t tryAccept(server_t server);
// módulo p/ aceitar uma nova conexão com um cliente

void setupSHM(client_t *client, bool flag);
// módulo p/ configurar o espaço de memória compartilhado

bool checkGroup(client_t client);
// módulo p/ verificar se o grupo do cliente existe

void handleSIGINT(int signal);
// função p/ tratar o sinal de interrupção (SIGINT)

void handleSIGCHLD(int signal);
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

void handleSIGTERM(int signal);
// função p/ tratar o sinal de encerramento de processo (SIGTERM)

void gracefulShutdown(int context);
// rotina de encerramento gracioso

void crashLanding(int context, char *error);
// rotina de encerramento em caso de falha

void killOffspring();
// função p/ matar os processos filhos

int updateGroup(long secret, char action);
// função p/ atualizar o contador dos grupos

int main(int argc, char **argv){
// uso: ./server <port>

    char error[BUFFER_SIZE];
    char path[64];

    // configura o tratamento de sinais...
    signal(SIGINT,  handleSIGINT);
    signal(SIGCHLD, handleSIGCHLD);

    server_t server = setupComm(argc, argv);

    printf("Servidor on-line e ouvindo na porta %d!\n", server.port);

    while(running){
    // loop do servidor

        client_t client = tryAccept(server);

        updateGroup(client.secret, '+'); // atualiza o contador de membros no grupo

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

            // configura o espaço de memória compartilhado
            setupSHM(&client, checkGroup(client));
            
            // lógica de comunicação
            #pragma omp parallel sections shared(running) firstprivate(client)
            {
                #pragma omp section
                {
                // entrada

                    while(running){
                    // recebe mensagens do cliente e encaminha para os outros membros do grupo

                        message_t message;
                        resetMsg(&message);

                        message_t aux_msg;
                        resetMsg(&aux_msg);

                        // recebe a mensagem do cliente
                        ssize_t rcvd = recv(client_fd,
                                            &message,
                                            sizeof(message_t),
                                            0);
                        if(rcvd <= 0){
                            if(rcvd == 0){
                            // a conexão foi perdida

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

                        // lê a quantidade de membros no grupo
                        int qty = 0;
                        sprintf(path, "./tmp/qty_%ld", client.secret);
                        FILE *f = fopen(path, "r");
                        fscanf(f, "%d", &qty);
                        fclose(f);

                        int curr_qty = -1;
                        while(curr_qty < qty){
                            // entra na seção crítica
                            if(sem_wait(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao entrar na seção crítica: ");

                                crashLanding(1, error);
                            }

                            // acessa o espaço de memória compartilhado
                            client.shm_ptr = (message_t*) shmat(client.shm_id, NULL, 0);
                            if(client.shm_ptr == (message_t*) -1){
                                FORMAT_ERROR(error, "Falha ao anexar o segmento de memória compartilhado: ");

                                crashLanding(1, error);
                            }

                            // lê a mensagem no espaço de memória compartilhado
                            aux_msg = *client.shm_ptr;
                            if(aux_msg.counter == -1){
                            // não há mensagem válida no espaço de memória compartilhado
                            
                                qty = -1;

                                // escreve a nova mensagem na memória compartilhada
                                *client.shm_ptr = message;
                            }else{
                                curr_qty = aux_msg.counter;

                                // somente se todos os membros do grupo receberam a mensagem...
                                if(curr_qty == qty){
                                    // substitui a mensagem na memória compartilhada
                                    *client.shm_ptr = message;
                                }
                            }

                            // libera o segmento de memória compartilhado
                            shmdt(client.shm_ptr);

                            // sai da seção crítica
                            if(sem_open(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                                crashLanding(1, error);
                            }

                            if(curr_qty < qty)usleep(250000); // espera 250ms antes da próx. tentativa
                        }
                    }
                }

                #pragma omp section
                {
                // saída

                    message_t old_msg;
                    resetMsg(&old_msg);

                    while(running){
                    // lê mensagens dos outros membros do grupo e encaminha para o cliente

                        message_t message;
                        resetMsg(&message);

                        // entra na seção crítica
                        if(sem_wait(client.sem_id) == false){
                            FORMAT_ERROR(error, "Falha ao entrar na seção crítica: ");

                            crashLanding(1, error);
                        }

                        // acessa o espaço de memória compartilhado
                        client.shm_ptr = (message_t*) shmat(client.shm_id, NULL, 0);
                        if(client.shm_ptr == (message_t*) -1){
                            FORMAT_ERROR(error, "Falha ao acessar o segmento de memória compartilhado: ");

                            crashLanding(1, error);
                        }

                        // lê a mensagem no espaço de memória compartilhado
                        message = *client.shm_ptr;

                        // verifica se a mensagem é válida
                        if(message.counter == -1){

                            // libera o segmento de memória compartilhado
                            shmdt(client.shm_ptr);

                            // sai da seção crítica
                            if(sem_open(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                                crashLanding(1, error);
                            }

                            usleep(250000); // espera 250ms antes da próx. tentativa

                            continue; // não há mensagem válida disponível...
                        }

                        // verifica se a mensagem é repetida
                        if(compareMsg(message, old_msg)){

                            // libera o segmento de memória compartilhado
                            shmdt(client.shm_ptr);

                            // sai da seção crítica
                            if(sem_open(client.sem_id) == false){
                                FORMAT_ERROR(error, "Falha ao sair da seção crítica: ");

                                crashLanding(1, error);
                            }

                            usleep(250000); // espera 250ms antes da próx. tentativa

                            continue; // a mensagem é repetida...
                        }else{
                        // é uma nova mensagem!
                        
                            old_msg = message;

                            // somente se a mensagem for de outro membro do grupo...
                            if(strcmp(message.username, client.username) != 0){

                                // atualiza o contador da mensagem
                                message.counter++;
                                *client.shm_ptr = message;
                            }
                        }

                        // libera o segmento de memória compartilhado
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

    char error[BUFFER_SIZE];

    struct sockaddr_in server_addr;
    struct sockaddr *server_addr_ptr
    = (struct sockaddr*)&server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
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
    server_addr.sin_addr.s_addr = INADDR_ANY;  // de qualquer origem e
    server_addr.sin_port = htons(server.port); // na porta escolhida

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
// módulo p/ aceitar uma nova conexão com um cliente

    char error[BUFFER_SIZE];

    while(running){
        client_fd = accept(server_fd,
                           server.server_addr_ptr,
                           &server.server_addr_len);
        if(client_fd < 0){
            sleep(1); // espera 1s antes da pŕox. tentativa

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
            // a conexão foi perdida

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

void setupSHM(client_t *client, bool flag){
// módulo p/ configurar o espaço de memória compartilhado

    char error[BUFFER_SIZE];
    char path[64];

    // inicia os espaços de memória compartilhados
    sprintf(path, "./tmp/shm_%ld", client->secret);
    FILE *f = fopen(path, "a");
    if(f)fclose(f);

    client->shm_key = ftok(path, PROJECT_ID);

    sprintf(path, "./tmp/sem_%ld", client->secret);
    f = fopen(path, "a");
    if(f)fclose(f);

    client->sem_key = ftok(path, PROJECT_ID);
            
    if(flag == false){
    // é o processo que cria o segmento de memória compartilhado
                
        client->shm_id = shmget(client->shm_key, SHM_SIZE, IPC_CREAT | 0666);
        if(client->shm_id < 0){
            FORMAT_ERROR(error, "Falha na criação do espaço de memória compartilhado: ");

            crashLanding(1, error);
        }

        client->sem_id = semget(client->sem_key, 1, IPC_CREAT | 0666);
        if(client->sem_id < 0){
            FORMAT_ERROR(error, "Falha na criação do semáforo compartilhado: ");

            crashLanding(1, error);
        }

        union semun sem_attr;
        sem_attr.val = 1;
        if(semctl(client->sem_id, 0, SETVAL, sem_attr) == -1){
            FORMAT_ERROR(error, "Falha ao atribuir o valor inicial ao semáforo: ");

            crashLanding(1, error);
        }

        // insere a primeira mensagem no espaço de memória compartilhado
        message_t tmp_msg;
        resetMsg(&tmp_msg);

        client->shm_ptr = (message_t*) shmat(client->shm_id, NULL, 0);
        if(client->shm_ptr == (message_t*) -1){
            FORMAT_ERROR(error, "Falha ao anexar o segmento de memória compartilhado: ");

            crashLanding(1, error);
        }

        *client->shm_ptr = tmp_msg;

        shmdt(client->shm_ptr); // libera o segmento de memória compartilhado
    }else{
    // é o processo que acessa o segmento de memória compartilhado

        client->shm_id = shmget(client->shm_key, SHM_SIZE, 0666); // sem IPC_CREAT
        if(client->shm_id < 0){
            FORMAT_ERROR(error, "Falha no acesso ao espaço de memória compartilhado: ");

            crashLanding(1, error);
        }

        client->sem_id = semget(client->sem_key, 1, 0666); // sem IPC_CREAT
        if(client->sem_id < 0){
            FORMAT_ERROR(error, "Falha no acesso ao semáforo compartilhado: ");

            crashLanding(1, error);
        }
    }
}

bool checkGroup(client_t client){
// módulo p/ verificar se o grupo do cliente existe

    bool flag = false;

    // percorre o array de grupos e verifica se o segredo existe
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

    return flag;
}

void handleSIGINT(int signal){
// função p/ tratar o sinal de interrupção (SIGINT)

    printf("\nSinal de interrupção recebido.\n"
           "\nEncerrando aplicação...\n");

    gracefulShutdown(0);

    exit(EXIT_SUCCESS);
}

void handleSIGCHLD(int signal){
// função p/ tratar o SIGCHLD (quando um processo filho encerra)

    int status;
    pid_t pid;
    int flag;
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

                flag = updateGroup(client.secret, '-'); // atualiza o contador do grupo

                if(flag == 1){
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
// função p/ tratar o sinal de encerramento de processo (SIGTERM)

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
            while(children_qty > 0); // espera todos os processos filhos encerrarem
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

int updateGroup(long secret, char action){
// função p/ atualizar o contador dos grupos

    FILE *f;
    int qty;

    char path[64];

    sprintf(path, "./tmp/qty_%ld", secret);

    switch(action){
        case '+':
        // incrementa...

            f = fopen(path, "r");
            if(f == NULL){
            // é o primeiro membro do grupo

                f = fopen(path, "w");
                qty = 1;
                fprintf(f, "%d\n", qty);
                fclose(f);
            }else{
            // é um novo membro no grupo

                fscanf(f, "%d", &qty);
                fclose(f);
                f = fopen(path, "w");
                qty++;
                fprintf(f, "%d\n", qty);
                fclose(f);
            }

            return 0;

            break;

        case '-':
        // decrementa...

            f = fopen(path, "r");
            fscanf(f, "%d", &qty);
            fclose(f);
            qty--;
            if(qty > 0){

                f = fopen(path, "w");
                fprintf(f, "%d\n", qty);
                fclose(f);
            }else{
            // era o último membro do grupo
            
                remove(path); // apaga o arquivo
            
                return 1; // notifica que o grupo está vazio
            }

            return 0;

            break;
    }
}