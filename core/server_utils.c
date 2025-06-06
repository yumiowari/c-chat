/*
 *  Bibliotecas
 */
#include <stdio.h>   // fprintf()
#include <stdbool.h> // bool type
#include <string.h>  // strlen()

#include "server_utils.h"

/*
 *  Funções
 */
bool checkServerArgs(int argc, char **argv){
// função p/ verificar os parâmetros de entrada
    
    bool flag = true;

    if(argc == 2){
        for(int i = 0; i < strlen(argv[1]); i++){
            if(argv[1][i] < '0' || argv[1][i] > '9'){
                fprintf(stderr, "A porta deve ser um número inteiro.\n");

                flag = false;

                break;
            }
        }
    }else{
        fprintf(stderr, "Parâmetros inválidos.\n"
                        "Uso: ./server <porta>\n");

        flag = false;
    }

    return flag;
}