// BIBLIOTECAS //

// padrão
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
//

// rede
#include <arpa/inet.h>
//

// processamento paralelo
#include <unistd.h>
#include <pthread.h>
//

/////////////////



// MACROS //

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

////////////



// ASSINATURAS //

bool checkArgs(int argc, char **argv);
// checa se os parâmetros da função main são válidos

void *handleMsgIn(void *arg);
// lida com o recebimento de mensagens do servidor

void *handleMsgOut(void *arg);
// lida com o envio de mensagens ao servidor

/////////////////



int main(int argc, char **argv){
    // VARIÁVEIS //

    unsigned short int port; // porta (0 - 65535)
    char username[16]; // nome de usuário
    int client_socket; // soquete de cliente
    struct sockaddr_in server_addr; // endereço do servidor
    char buffer[BUFFER_SIZE]; // buffer para I/O de mensagem

    ///////////////



    // ALGORITMO //

    if(checkArgs(argc, argv)){
        printf("Verificação de parâmetros de inicialização bem-sucedida.\n");

        port = atoi(argv[1]);
        strcpy(username, argv[2]);
    }else{
        fprintf(stderr, "Verificação de parâmetros de inicialização falhou.\n");

        exit(EXIT_FAILURE);
    }

    // criando soquete de cliente
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // TCP e IPv4
    if(client_socket == -1){
        fprintf(stderr, "Falha na criação do soquete do cliente.\n");

        exit(EXIT_FAILURE);
    }else printf("Criação do soquete do cliente bem-sucedida.\n");
    //

    // configurando endereço do servidor
    printf("Configurando o endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);
    //

    // conectando ao servidor
    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, "Falha ao conectar ao servidor.\n");

        exit(EXIT_FAILURE);
    }else printf("\nConexão estabelecida com o servidor!\n");
    //

    // informa o nome de usuário ao servidor
    strcpy(buffer, username);
    if(send(client_socket, buffer, BUFFER_SIZE, 0) == -1){
        fprintf(stderr, "Falha ao informar o nome de usuário ao servidor.\n");

        exit(EXIT_FAILURE);
    }
    //

    pthread_t tid_in, tid_out;

    // lógica de comunicação com o servidor
    if(pthread_create(&tid_in, NULL, handleMsgIn, &client_socket) != 0){
        fprintf(stderr, "Falha ao criar thread para escutar o servidor.\n");

        exit(EXIT_FAILURE);
    }

    if(pthread_create(&tid_out, NULL, handleMsgOut, &client_socket) != 0){
        fprintf(stderr, "Falha ao criar thread para falar ao servidor.\n");

        exit(EXIT_FAILURE);
    }

    if(pthread_join(tid_in, NULL) != 0){ // espera o fim da conexão com o servidor
        fprintf(stderr, "Falha ao aguardar a thread handleMsgIn().\n");

        exit(EXIT_FAILURE);
    }
    //

    printf("Encerrando cliente...\n");

    exit(EXIT_SUCCESS);

    ///////////////
}



// FUNÇÕES //

bool checkArgs(int argc, char **argv){
    // VARIÁVEIS //

    int i; // contador
    bool flag = true; // bandeira

    ///////////////



    // ALGORITMO //

    if(argc < 3){
        fprintf(stderr, "Argumentos insuficientes.\nUso: ./server <porta> <nome de usuário>\n");

        flag = false;
    }else if(argc > 3){
        fprintf(stderr, "Argumentos demais.\nUso: ./server <porta> <nome de usuário>\n");

        flag = false;
    }else{
        // verifica se a porta é inválida
        for(i = 0; i < strlen(argv[1]); i++){
            if((argv[1][i] < 48) || (argv[1][i] > 57)){
                flag = false;
            
                break;
            }
        }

        if((atoi(argv[1]) > 65535) || (atoi(argv[1]) < 0))flag = false;

        if(flag == false)fprintf(stderr, "A porta deve ser um número inteiro entre 0 e 65535.\n");
        //

        // verifica se o nome de usuário é inválido
        if(strlen(argv[2]) > 15){
            fprintf(stderr, "O nome de usuário não pode exceder 15 caracteres.\n");

            flag = false;
        }

        for(i = 0; i < strlen(argv[2]); i++){
            if((argv[2][i] < 65) || (argv[2][i] > 90) && (argv[2][i] < 97) || (argv[2][i] > 122) && (argv[2][1] != 95)){
                fprintf(stderr, "O nome de usuário só pode conter caracteres válidos:\nA-Z a-z _\n");

                flag = false;

                break;
            }
        }
        //
    }
    
    return flag;

    ///////////////
}

void *handleMsgIn(void *arg){
    // VARIÁVEIS //

    char buffer[BUFFER_SIZE]; // buffer para I/O de mensagem
    ssize_t recv_bytes; // qtd de bytes recebidos
    int client_socket = *((int*) arg); // soquete de cliente

    ///////////////



    // ALGORITMO //

    while(true){
        recv_bytes = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        if(recv_bytes <= 0){
            if(recv_bytes == 0){
                printf("Conexão com servidor foi perdida.\n");
            }else fprintf(stderr, "Falha na recepção de dados.\n");

            break;
        }

        buffer[recv_bytes] = '\0';

        if(strncmp(buffer, "Ok!", 3) != 0){
            fprintf(stderr, "O servidor terminou a conexão.\n");

            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);

    ///////////////
}

void *handleMsgOut(void *arg){
    // VARIÁVEIS //

    char buffer[BUFFER_SIZE]; // buffer para I/O de mensagem
    int client_socket = *((int*) arg); // soquete de cliente

    ///////////////



    // ALGORITMO //

    while(true){
        printf("> ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if(send(client_socket, buffer, BUFFER_SIZE, 0) == -1){
            fprintf(stderr, "Falha ao enviar mensagem ao servidor.\n");

            break;
        }

        memset(buffer, 0, BUFFER_SIZE); // limpa o buffer
    }

    close(client_socket);

    pthread_exit(NULL);

    ///////////////
}

/////////////