#include <stdio.h>     // fprintf()
#include <stdbool.h>   // bool type
#include <string.h>    // strlen()

#include "client_utils.h"

/*
 *  Funções
 */
bool checkClientArgs(int argc, char **argv){
// função p/ verificar os parâmetros de inicialização

    bool flag = true;

    if(argc == 4){
        if(strlen(argv[1]) > 15){
            fprintf(stderr, "O nome de usuário não pode exceder 15 caracteres.\n");

            flag = false;
        }

        if(flag == true){
            // verifica o nome de usuário
            for(int i = 0; i < strlen(argv[1]); i++){
                if(argv[2][i] >= 'A' && argv[2][i] <= 'Z'){
                    // A-Z
                }else if(argv[2][i] >= 'a' && argv[2][i] <= 'z'){
                    // a-z
                }else if(argv[2][i] != '_'){
                    // _
                }else{
                    fprintf(stderr, "Caracteres inválidos para nome de usuário.\n"
                                    "Uso: A-Z a-z _\n");

                    flag = false;

                    break;
                }
            }
        }

        if(flag == true){
            // verifica o segredo
            for(int i = 0; i < strlen(argv[2]); i++){
                if(argv[2][i] < '0' || argv[2][i] > '9'){
                    fprintf(stderr, "O segredo deve ser um número inteiro.\n");

                    flag = false;

                    break;
                }
            }
        }

        if(flag == true){
            // verifica a porta
            for(int i = 0; i < strlen(argv[3]); i++){
                if(argv[3][i] < '0' || argv[3][i] > '9'){
                    fprintf(stderr, "A porta deve ser um número inteiro.\n");

                    flag = false;

                    break;
                }
            }
        }
    }else{
        fprintf(stderr, "Parâmetros inválidos.\n"
                        "Uso: ./client <username> <segredo> <porta>\n");

        flag = false;
    }

    return flag;
}

bool compareMsg(message_t A, message_t B){
// função p/ verificar se A == B

    if(strcmp(A.username, B.username) == 0 &&
       strcmp(A.buffer, B.buffer) == 0){
    // se o remetente e o conteúdo da mensagem forem iguais...

        return true;
    }

    return false;
}

void debugMsg(message_t msg){
// função p/ imprimir os atributos da mensagem

    printf("Remetente: %s\n"
           "Grupo:     %ld\n"
           "Conteúdo:  %s\n"
           "Contador:  %d\n", msg.username,
                              msg.secret,
                              msg.buffer,
                              msg.counter);
}

void resetMsg(message_t *msg){
// função p/ resetar os atributos da mensagem

    memset(msg->username, 0, 16);
    memset(msg->buffer, 0, 1024);
    msg->secret = -1;
    msg->counter = -1;
}