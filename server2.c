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

#define RESET  "\x1b[0m"
#define RED    "\x1b[31m"
#define GREEN  "\x1b[32m"
#define YELLOW "\x1b[33m"

bool checkArgs(int argc, char **argv);
// função p/ verificar parâmetros de entrada

int main(int argc, char **argv){
// uso: ./server <porta>

    printf("Verificando parâmetros de entrada...\n");
    if(checkArgs(argc, argv)){

    }else exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}

bool checkArgs(int argc, char **argv){
    if(argc < 2){
        fprintf(stderr, RED "ERRO: Argumentos insuficientes.\n" RESET \
                            "Uso: ./server <porta>\n");

        return false;
    }else if(argc > 2){
        fprintf(stderr, YELLOW "AVISO: Argumentos excedentes.\n" RESET \
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