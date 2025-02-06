//////////////////////////////////////////////////
//                                              //
//     Sistema de Chatting Cliente-Servidor     //
//                                              //
//         Por Rafael Renó Corrêa, 2025         //
//                                              //
//////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <arpa/inet.h>

#define RESET  "\x1b[0m"
#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"

#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

bool checkArgs(int argc, char **argv);
// função p/ verificar parâmetros de entrada

int main(int argc, char **argv){
// uso: ./client <porta> <nome de usuário>

    unsigned short int port; // porta (0 - 65535)
    char username[16]; // nome de usuário
    int client_socket; // soquete de cliente
    struct sockaddr_in server_addr; // endereço do servidor
    char buffer[BUFFER_SIZE]; // buffer para I/O

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){
        port = atoi(argv[1]);
        strcpy(username, argv[2]);
    }else exit(EXIT_FAILURE);

    printf("Criando soquete de cliente...\n");
    client_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(client_socket == -1){
        fprintf(stderr, RED "ERRO: Falha na criação do soquete do servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    printf("Configurando endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);

    printf("Estabelecendo conexão com o servidor...\n");
    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: Falha ao estabelecer conexão com o servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nConexão estabelecida com o servidor!\n" RESET);

    // informa o nome de usuário ao servidor
    strcpy(buffer, username);
    if(send(client_socket, buffer, BUFFER_SIZE, 0) == -1){
        fprintf(stderr, RED "ERRO: Falha ao informar o nome de usuário ao servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

bool checkArgs(int argc, char **argv){
    if(argc < 3){
        fprintf(stderr, RED "ERRO: Argumentos insuficientes.\n" RESET
                            "Uso: ./client <porta> <nome de usuário>\n");

        return false;
    }else if(argc > 3){
        fprintf(stderr, YELLOW "AVISO: Argumentos excedentes.\n" RESET
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