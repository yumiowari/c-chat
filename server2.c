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

bool checkArgs(int argc, char **argv);
// função p/ verificar parâmetros de entrada

int main(int argc, char **argv){
// uso: ./server <porta>

    unsigned short int port; // porta (0 - 65535)
    int server_socket; // soquete do servidor
    struct sockaddr_in server_addr; // endereço do servidor

    printf("Verificando parâmetros de entrada...\n");
    if(!checkArgs(argc, argv))exit(EXIT_FAILURE);

    printf("Criando soquete do servidor...\n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0); // soquete TCP/IPv4
    if(server_socket == -1){
        fprintf(stderr, RED "ERRO: Falha na criação do soquete do servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }

    printf("Configurando endereço do servidor...\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // aceita requisições de qualquer IP
    port = atoi(argv[1]);
    server_addr.sin_port = htons(port);

    printf("Vinculando o soquete do servidor à porta %d...\n", port);
    if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, RED "ERRO: Falha ao vincular o soquete do servidor à porta %d." RESET, port);

        exit(EXIT_FAILURE);
    }

    printf("Iniciando o servidor...\n");
    if(listen(server_socket, 5) == -1){
        fprintf(stderr, RED "Falha ao iniciar o servidor.\n" RESET);

        exit(EXIT_FAILURE);
    }else printf(GREEN "\nServidor on-line e ouvindo na porta %d!\n", port);

    // loop do servidor
    while(true){

    }

    exit(EXIT_SUCCESS);
}

bool checkArgs(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr, RED "ERRO: Argumentos insuficientes.\n" RESET
                            "Uso: ./server <porta>\n");

        return false;
    }else if(argc > 2){
        fprintf(stderr, YELLOW "AVISO: Argumentos excedentes.\n" RESET
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