#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "client_utils.h"

/*
 *  Funções
 */
bool checkArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada

    bool flag = true;

    if(argc == 3){
        if(strlen(argv[1]) > 15){
            fprintf(stderr, "O nome de usuário não pode exceder 15 caracteres.\n");

            flag = false;
        }

        if(flag == true){
            // verifica o nome de usuário
            for(int i = 0; i < strlen(argv[1]); i++){
                if(
                   argv[2][i] < 'A' && argv[2][i] > 'Z' &&
                   argv[2][i] < 'a' && argv[2][i] > 'z' &&
                   argv[2][i] != '_'
                  ){
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
                    flag = false;

                    break;
                }
            }
        }
    }else{
        fprintf(stderr, "Parâmetros inválidos.\n"
                        "Uso: ./client <username>\n");

        flag = false;
    }

    return flag;
}